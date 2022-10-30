/*Copyright 2022 Eric Baxter <ericwbaxter85@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining 
a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software 
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

File:  icm20948.c

Implements support for the Bosch BME 280 temperature, pressure and humidity chip.

Note:  The conversion algorithms were taken directly from the BME 280 spec sheet.

*/

#include <stdio.h>
#include "pico/stdlib.h"

#include "icm20948.h"
#include "Icm20948_Inv.h"
#include "Icm20948Dmp3Driver.h"

#define ICM20948_SUPPORTED_DEVICE_COUNT ACCELEROMETER_NUMBER_SUPPORTED_DEVICES
#define ICM20948_REGISTER_RW_SIZE	2

#define ICM20948_BANK_0	0x00
#define ICM20948_BANK_1	0x10
#define ICM20948_BANK_2	0x20
#define ICM20948_BANK_3	0x30

#define ICM20948_WHO_AM_I_REGISTER	0x00
#define ICM20948_LP_CONFIG_REGISTER     0x05
#define ICM20948_POWER_MANAGEMENT_1_REGISTER 0x06
#define ICM20948_USER_CONTROL_REGISTER	0x6A
#define ICM20948_REG_BANK_SEL_REGISTER	0x7F

#define ICM20948_BIT_ACCEL_CYCLE    0x20
#define ICM20948_BIT_GYRO_CYCLE     0x10

#define ICM20948_WHO_AM_I_VALUE		0xEA
#define ICM20948_POWER_MGMT_RESET   0x80
#define ICM20948_SLEEP_BIT			0x40
#define ICM20948_LP_ENABLE_BIT		0x20
#define ICM20948_SLAVE_I2C_DISABLE	0x10

#define ICM20948_DMP_LOAD_START    	0x90

#define ICM20948_RESET_WAIT         100
#define ICM20948_INV_MAX_SERIAL_READ 		2
#define ICM20948_INV_MAX_SERIAL_WRITE 		2
#define ICM20948_INVALID_DMP_BANK	0xFF

typedef struct ICM20948_Params {
	uint32_t chip_select;
	spi_inst_t *spi;
	uint8_t firmware_loaded;
} ICM20948_Parameters;

static ICM20948_Parameters icm20948_params[ICM20948_SUPPORTED_DEVICE_COUNT];
static uint32_t number_icm20948_initialized = 0;

static const unsigned char dmp3_image[] = {
#include "icm20948_img.dmp3a.h"
};

#define READ_BIT 0x80

static inline void cs_select(uint32_t chip_select) {
    asm volatile("nop \n nop \n nop");
    gpio_put(chip_select, 0);  // Active low
	sleep_us(10);
    //asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(uint32_t chip_select) {
    asm volatile("nop \n nop \n nop");
    gpio_put(chip_select, 1);
    asm volatile("nop \n nop \n nop");
}

static Error_Returns icm20948_set_register_bank(ICM20948_Parameters *params_ptr, uint8_t register_bank)
{
	static last_bank_set = 0xFF;
	Error_Returns to_return = RPi_Success;
	
	cs_select(params_ptr->chip_select);
	
	do
	{
		uint8_t buffer[ICM20948_REGISTER_RW_SIZE];
		if (last_bank_set == register_bank)
		{
			break;
		}
		
		buffer[0] = ICM20948_REG_BANK_SEL_REGISTER;
		buffer[1] = register_bank;
		
		if (spi_write_blocking(params_ptr->spi, &buffer, ICM20948_REGISTER_RW_SIZE) ==
			PICO_ERROR_GENERIC)
		{
			printf("icm20948_write reg write 0x%x\n", reg);	
			to_return = RPi_OperationFailed;
		}
		
	} while(0);
	
	cs_deselect(params_ptr->chip_select);
	return to_return;
}

//Communication routine with the ICM 20948
static int icm20948_write(void *param_ptr, uint8_t reg, const uint8_t *buffer, uint32_t tx_bytes)
{	
	int to_return = 0;
	ICM20948_Parameters *params_ptr = (ICM20948_Parameters *)param_ptr;
	cs_select(params_ptr->chip_select);
	do
	{
		if (spi_write_blocking(params_ptr->spi, &reg, 1) ==
		PICO_ERROR_GENERIC)
		{
			printf("icm20948_write reg write 0x%x\n", reg);
			to_return = -1;
			break;
		}
		
		if (spi_write_blocking(params_ptr->spi, buffer, tx_bytes) ==
			PICO_ERROR_GENERIC)
		{
			printf("icm20948_write buffer write %u\n", tx_bytes);
			to_return = -1;
		}
	} while(0);
	cs_deselect(params_ptr->chip_select);
	return to_return;
}


//Communication routine with the ICM 20948
static int icm20948_read(void *param_ptr, uint8_t reg, uint8_t *buffer, uint32_t rx_bytes)
{
	int to_return = 0;
	ICM20948_Parameters *params_ptr = (ICM20948_Parameters *)param_ptr;
	cs_select(params_ptr->chip_select);
	do
	{
		reg |= READ_BIT;
		if (spi_write_blocking(params_ptr->spi, &reg, 1) ==
		PICO_ERROR_GENERIC)
		{
			printf("icm20948_read reg write 0x%x\n", reg);
			to_return = -1;
			break;
		}
		
		if (spi_read_blocking(params_ptr->spi, 0x0, buffer, rx_bytes) ==
			PICO_ERROR_GENERIC)
		{
			printf("icm20948_write buffer write %u\n", rx_bytes);
			to_return = -1;
		}
	} while(0);
	cs_deselect(params_ptr->chip_select);
	return to_return;
}

static Error_Returns icm20948_write_register(ICM20948_Parameters *params_ptr, uint8_t reg_bank, uint8_t reg, const uint8_t register_value)
{
	Error_Returns to_return = icm20948_set_register_bank(params_ptr, reg_bank);
	
	cs_select(params_ptr->chip_select);
	
	do
	{
		if (to_return != RPi_Success)
		{
			break;
		}
		
		uint8_t buffer[ICM20948_REGISTER_RW_SIZE];
		
		buffer[0] = reg;
		buffer[1] = register_value; 
		if (spi_write_blocking(params_ptr->spi, &buffer, ICM20948_REGISTER_RW_SIZE) ==
		PICO_ERROR_GENERIC)
		{
			printf("icm20948_write_register: reg write failed\n");
			to_return = RPi_OperationFailed;
		}
	} while(0);
	
	cs_deselect(params_ptr->chip_select);
	return to_return;
}

static Error_Returns icm20948_read_register(ICM20948_Parameters *params_ptr, uint8_t reg_bank, uint8_t reg, const uint8_t *register_value)
{
	Error_Returns to_return = icm20948_set_register_bank(params_ptr, reg_bank);
	
	cs_select(params_ptr->chip_select);
	do
	{
		if (to_return != RPi_Success)
		{
			break;
		}
		
		if (spi_write_blocking(params_ptr->spi, &reg, 1) ==
		PICO_ERROR_GENERIC)
		{
			printf("icm20948_read_register reg write 0x%x\n", reg);
			to_return = RPi_OperationFailed;
			break;
		}
		
		if (spi_read_blocking(params_ptr->spi, register_value, 1) ==
			PICO_ERROR_GENERIC)
		{
			printf("icm20948_read_register read failed\n");
			to_return = RPi_OperationFailed;
		}
	} while(0);
	
	cs_deselect(params_ptr->chip_select);
	return to_return;
}

//Begin convert
static int inv_icm20948_write_mems(ICM20948_Parameters *params_ptr, unsigned short reg, unsigned int length, const unsigned char *data)
{
    int result=0;
    unsigned int bytesWritten = 0;
    unsigned int thisLen;
    unsigned char lBankSelected;
    unsigned char lStartAddrSelected;
	static unsigned char lLastBankSelected;
            
	icm20948_set_register_bank(params_ptr, 0)
    
    lBankSelected = (reg >> 8);
	if (lBankSelected != s->lLastBankSelected)
	{
		result |= inv_icm20948_write_reg(s, REG_MEM_BANK_SEL, &lBankSelected, 1);

		if (result)
		{
			printf("inv_icm20948_write_mems:  write to %u (bank sel) of %u failed with %d\n",
			REG_MEM_BANK_SEL, lBankSelected, result);
			return result;
		}
		s->lLastBankSelected = lBankSelected;
	}
    
    while (bytesWritten < length) 
    {
        lStartAddrSelected = (reg & 0xff);
        /* Sets the starting read or write address for the selected memory, inside of the selected page (see MEM_SEL Register).
           Contents are changed after read or write of the selected memory.
           This register must be written prior to each access to initialize the register to the proper starting address.
           The address will auto increment during burst transactions.  Two consecutive bursts without re-initializing the start address would skip one address. */
        result |= inv_icm20948_write_reg(s, REG_MEM_START_ADDR, &lStartAddrSelected, 1);
        if (result)
		{
			printf("inv_icm20948_write_mems:  write to 0x%x (MEM_Start_addr) of 0x%x failed with %d\n",
			REG_MEM_START_ADDR, lStartAddrSelected, result);
			sleep_ms(10);
            return result;
		}
        
        thisLen = min(INV_MAX_SERIAL_WRITE, length-bytesWritten);
        
        /* Write data */ 
        result |= inv_icm20948_write_reg(s, REG_MEM_R_W, &data[bytesWritten], thisLen);
        if (result)
		{
			printf("inv_icm20948_write_mems:  write to %u (memory) of length %u failed with %d\n",
			REG_MEM_R_W, thisLen, result);
            return result;
		}
        
        bytesWritten += thisLen;
        reg += thisLen;
    }

    //Enable LP_EN since we disabled it at begining of this function.
    result |= inv_icm20948_set_chip_power_state(s, CHIP_LP_ENABLE, 1);

    return result;
}

Error_Returns icm20948_firmware_load(const unsigned char *data_start_address, unsigned short load_size, unsigned short dmp_load_address)
{
	Error_Returns to_return = RPi_Success;
	unsigned char last_bank = ICM20948_INVALID_DMP_BANK;
    int write_size;
    int result;
    unsigned short memaddr;
    const unsigned char *data;
    unsigned short size;
    unsigned char data_cmp[ICM20948_INV_MAX_SERIAL_READ];
    int flag = 0;
	int retry_count = 0;
	int mem_cmp_results = 0;
		
    // Write DMP memory
    data = data_start_address;
    size = load_size;
    memaddr = dmp_load_address;

    while (size > 0) {
        write_size = min(size, ICM20948_INV_MAX_SERIAL_WRITE);
        if ((memaddr & 0xff) + write_size > 0x100) {
            // Moved across a bank
            write_size = (memaddr & 0xff) + write_size - 0x100;
        }

		result = inv_icm20948_write_mems(s, memaddr, write_size, (unsigned char *)data);
		if (result) 
		{	
			printf("Write DMP memory, result = %d\n", result);
			//return result;
		}

        data += write_size;
        size -= write_size;
        memaddr += write_size;
    }

    // Verify DMP memory
    data = data_start;
    size = size_start;
    memaddr = load_addr;
    while (size > 0) {
        write_size = min(size, ICM20948_INV_MAX_SERIAL_READ);
        if ((memaddr & 0xff) + write_size > 0x100) {
            // Moved across a bank
            write_size = (memaddr & 0xff) + write_size - 0x100;
        }	
		

		result = inv_icm20948_read_mems(s, memaddr, write_size, &data_cmp[0]);
		if (result)
		{
			flag++; // Error, DMP not written correctly
			printf ("Verify result = %d flag = %d\n", result, flag);
		}
		
		mem_cmp_results = memcmp(data_cmp, data, write_size);
		if (mem_cmp_results != 0)
		{
			printf ("Verify data failed = %hhu dmp = %hhu \n", data_cmp[0], data[0]);
			return -1;
		}

		
        data += write_size;
        size -= write_size;
        memaddr += write_size;
    }
	return to_return;
}
//End convert

/* Assumes the SPI bus has been intialized.
*/
Error_Returns icm20948_init(uint32_t *id, spi_inst_t *spi, uint32_t chip_select)
{	
	Error_Returns to_return = RPi_Success;
	uint8_t register_val;

	if (number_icm20948_initialized < ICM20948_SUPPORTED_DEVICE_COUNT)
	{
		do
		{
			ICM20948_Parameters *params_ptr = &icm20948_params[number_icm20948_initialized];
			params_ptr->chip_select = chip_select;
			params_ptr->spi = spi;
			params_ptr->firmware_loaded = 0;
			
			to_return = icm20948_read_register(params_ptr, ICM20948_BANK_0, ICM20948_WHO_AM_I_REGISTER, 
				&register_val);
			if (to_return != RPi_Success)
			{
				printf("icm20948_init():  Error reading chip ID read was %u\n", to_return);
				break;  //No need to continue just return the failure
			}
			if (register_val != ICM20948_WHO_AM_I_VALUE)
			{
				printf("icm20948_init():  Error chip ID read was 0x%x\n", register_val);
				to_return = RPi_OperationFailed;
				break;
			}

			/* Reset the chip */
			to_return = icm20948_write_register(params_ptr, ICM20948_BANK_0,
				ICM20948_POWER_MANAGEMENT_1_REGISTER, ICM20948_POWER_MGMT_RESET);
			if (to_return != RPi_Success)
			{
				printf("icm20948_init():  Error resetting chip\n");
				break;  //No need to continue just return the failure
			}
			sleep_ms(ICM20948_RESET_WAIT);
			
			/* When it comes out of reset the chip is  in sleep mode,
			low power enable is set, the clock used is the best available
			and the accelerometers and gyroscopes are disabled.  We need
			to take it out of sleep mode and disable the low power mode */
			
			to_return = icm20948_read_register(params_ptr, ICM20948_BANK_0
				ICM20948_POWER_MANAGEMENT_1_REGISTER, &register_val);
			if (to_return != RPi_Success)
			{
				printf("icm20948_init():  Error reading chip ID ICM20948_POWER_MANAGEMENT_1_REGISTER\n");
				break;  //No need to continue just return the failure
			}
			
			register_val &= ~ICM20948_SLEEP_BIT;
			register_val &= ~ICM20948_LP_ENABLE_BIT;
			to_return = icm20948_write_register(params_ptr, ICM20948_BANK_0
				ICM20948_POWER_MANAGEMENT_1_REGISTER, register_val);
			if (to_return != RPi_Success)
			{
				printf("icm20948_init():  Error taking chip out of sleep/lp_enable\n");
				break;  //No need to continue just return the failure
			}			
						
			register_val = ICM20948_BIT_ACCEL_CYCLE | ICM20948_BIT_GYRO_CYCLE;

			to_return = icm20948_write_register(params_ptr, ICM20948_BANK_0
				ICM20948_LP_CONFIG_REGISTER, register_val);
			if (to_return != RPi_Success)
			{
				printf("icm20948_init():  Error writing ICM20948_LP_CONFIG_REGISTER\n");
				break;  //No need to continue just return the failure
			}	
			

			register_val = ICM20948_SLAVE_I2C_DISABLE;
			
			to_return = icm20948_write_register(params_ptr, ICM20948_BANK_0
				ICM20948_USER_CONTROL_REGISTER, register_val);
			if (to_return != RPi_Success)
			{
				printf("icm20948_init():  Error writing ICM20948_USER_CONTROL_REGISTER\n");
				break;  //No need to continue just return the failure
			}
			
			to_return = icm20948_firmware_load(&dmp3_image[0], sizeof(dmp3_image),
				ICM20948_DMP_LOAD_START);
			
			if (to_return != RPi_Success)
			{
				printf("icm20948_init:  Failed to load DMP\n");
				to_return = RPi_OperationFailed;
				break;
			}
			
			/* Begin convert */

			result |= inv_set_dmp_address();

			result |= dmp_reset_control_registers();
			
			// set FIFO watermark to 80% of actual FIFO size
			result |= dmp_set_FIFO_watermark(800);

			// Enable Interrupts.
			data = 0x2;
			result |= inv_write_mems_reg(REG_INT_ENABLE, 1, &data); // Enable DMP Interrupt
			data = 0x1;
			result |= inv_write_mems_reg(REG_INT_ENABLE_2, 1, &data); // Enable FIFO Overflow Interrupt

			// TRACKING : To have accelerometers datas and the interrupt without gyro enables.
			data = 0XE4;
			result |= inv_write_mems_reg(REG_SINGLE_FIFO_PRIORITY_SEL, 1, &data);

			// Disable HW temp fix
			inv_read_mems_reg(REG_HW_FIX_DISABLE,1,&data);
			data |= 0x08;
			inv_write_mems_reg(REG_HW_FIX_DISABLE,1,&data);

			// Setup MEMs properties.
			base_state.accel_averaging = 1; //Change this value if higher sensor sample avergaing is required.
			base_state.gyro_averaging = 1;  //Change this value if higher sensor sample avergaing is required.
			inv_set_gyro_divider(FIFO_DIVIDER);       //Initial sampling rate 1125Hz/10+1 = 102Hz.
			inv_set_accel_divider(FIFO_DIVIDER);      //Initial sampling rate 1125Hz/10+1 = 102Hz.
			result |= inv_set_gyro_fullscale(MPU_FS_2000dps);
			result |= inv_set_accel_fullscale(MPU_FS_2G);

			// FIFO Setup.
			result |= inv_write_single_mems_reg(REG_FIFO_CFG, BIT_SINGLE_FIFO_CFG); // FIFO Config. fixme do once? burst write?
			result |= inv_write_single_mems_reg(REG_FIFO_RST, 0x1f); // Reset all FIFOs.
			result |= inv_write_single_mems_reg(REG_FIFO_RST, 0x1e); // Keep all but Gyro FIFO in reset.
			result |= inv_write_single_mems_reg(REG_FIFO_EN, 0x0); // Slave FIFO turned off.
			result |= inv_write_single_mems_reg(REG_FIFO_EN_2, 0x0); // Hardware FIFO turned off.
			
			result |= inv_read_mems(MPU_SOFT_UPDT_ADDR, 1, &data);

			base_state.lp_en_support = 1;

			if(base_state.lp_en_support == 1)
				inv_set_chip_power_state(CHIP_LP_ENABLE, 1);

			result |= inv_sleep_mems();   
        
    			
			/* End convert */ 
			
			if (inv_icm20948_load_firmware(&params_ptr->icm20948_driver, &dmp3_image[0], sizeof(dmp3_image)) != 0)
			{
				printf("icm20948_init:  Failed to load DMP\n");
				to_return = RPi_OperationFailed;
				break;
			}
			
			*id = number_icm20948_initialized++;				 
		} while(0);
	}
	
	return to_return;
}

Error_Returns icm20948_reset(uint32_t id)
{
	return RPi_Success;
}

