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

File:  bme280.c

Implements support for the Bosch BME 280 temperature, pressure and humidity chip.

Note:  The conversion algorithms were taken directly from the BME 280 spec sheet.

*/

#include <stdio.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "bme280.h"

#define BME280_CHIP_ID 0x60
#define BME280_CHIP_RESET_WORD 0xB6

#define BME280_FIRST_TRIM_PARAMETER 0x88
#define BME280_SECOND_TRIM_PARAMETER 0xA1
#define BME280_THIRD_TRIM_PARAMETER 0xE1
#define BME280_CHIP_RPi_REGISTER 0xD0
#define BME280_CHIP_RESET_REGISTER 0xE0
#define BME280_CTRL_HUMIDITY_REGISTER 0xF2
#define BME280_STATUS_REGISTER 0xF3
#define BME280_CTRL_MEASURE_REGISTER 0xF4
#define BME280_CTRL_CONFIG_REGISTER 0xF5
#define BME280_FIRST_DATA_REGISTER 0xF7

#define BME280_CTRL_REGISTER_WRITE_SIZE 2
#define BME280_DATA_REGISTER_SIZE 0x6
#define BME280_TRIM_PARAMETER_BYTES 24

#define BME280_SLEEP_MODE 0
#define BME280_IIR_OFF_500MS_STANDBY 0x80
#define BME280_HUMIDITY_1X	0x01
#define BME280_HUMIDITY_OFF  0x00
#define BME280_PRESS_TEMP_1X  0x27
#define BME280_IIR_16_500MS_STANDBY 0x10
#define BME280_NO_IIR_16_500MS_STANDBY 0x0
#define BME280_PRESS8X_TEMP_1X	0x33
#define BME280_PRESS16X_TEMP_2X 0x53
#define BME280_PRESS1X_TEMP_1X 0x27

#define BME280_STATUS_MEASURING_BIT	3

#define TIME_DELAY 1
#define BME280_STATUS_READ_ATTEMPTS	10
#define BME280_UPPER_WORD_MASK		12
#define BME280_MIDDLE_WORD_MASK		4
#define BME280_IIR_ENABLED_MASK		4
#define BME280_IIR_DISABLED_1X_SAMPLING_MASK	0
#define BME280_REGISTER_BIT_SIZE	8

typedef struct Comp_Params {
	unsigned short dig_T1;
	signed short dig_T2;
	signed short dig_T3;

	unsigned short dig_P1;
	signed short dig_P2;
	signed short dig_P3;
	signed short dig_P4;
	signed short dig_P5;
	signed short dig_P6;
	signed short dig_P7;
	signed short dig_P8;
	signed short dig_P9;

	unsigned char dig_H1;
	signed short dig_H2;
	unsigned char dig_H3;
	signed short dig_H4;
	signed short dig_H5;
	char dig_H6;

	BME280_S32_t t_fine;
	uint32_t address;
} Compensation_Parameters;

static Compensation_Parameters bme280_compensation_params[BAROMETER_NUMBER_SUPPORTED_DEVICES];

static unsigned char bme280_ready = 0;

static uint32_t pressure_temperature_xlsb_mask = 0;

//Communication routine with the BME 280
static Error_Returns bme280_write(uint32_t address, unsigned char *buffer, unsigned int tx_bytes)
{	
	Error_Returns to_return = RPi_Success;
	if (i2c_write_blocking(i2c_default, address, buffer, tx_bytes, false) ==
		PICO_ERROR_GENERIC)
	{
		to_return = I2CS_Data_Loss;
	}
	return to_return;
}


//Communication routine with the BME 280
static Error_Returns bme280_read(uint32_t address, unsigned char *buffer, unsigned int rx_bytes)
{
	Error_Returns to_return = RPi_Success;
	to_return = bme280_write(address,  buffer, 1);
	if (to_return == RPi_Success)
	{
		if (i2c_read_blocking(i2c_default, address, buffer, rx_bytes, false) ==
			PICO_ERROR_GENERIC)
		{
			to_return = I2CS_Data_Loss;
		}
	}
	return to_return;
}

//Taken straight from the Bosch manual.
static double compensateTemperature(uint32_t id, int32_t adc_T)
{
  double v_x1_u32;
  double v_x2_u32;
  double temperature;
  
  Compensation_Parameters *params_ptr = &bme280_compensation_params[id];
  
  v_x1_u32  = (((double)adc_T) / 16384.0 - ((double)params_ptr->dig_T1) / 1024.0) * ((double)params_ptr->dig_T2);
  v_x2_u32  = ((((double)adc_T) / 131072.0 - ((double)params_ptr->dig_T1) / 8192.0) * (((double)adc_T) / 131072.0 - ((double)params_ptr->dig_T1) / 8192.0)) * ((double)params_ptr->dig_T3);
  params_ptr->t_fine = (BME280_S32_t)(v_x1_u32 + v_x2_u32);
  temperature  = (v_x1_u32 + v_x2_u32) / 5120.0;
  return temperature;
}

//Taken straight from the Bosch manual.
static double compensatePressure(uint32_t id, int32_t adc_P)
{
  double v_x1_u32;
  double v_x2_u32;
  double pressure;
  
  Compensation_Parameters *params_ptr = &bme280_compensation_params[id];
  
  v_x1_u32 = ((double)params_ptr->t_fine / 2.0) - 64000.0;
  v_x2_u32 = v_x1_u32 * v_x1_u32 * ((double)params_ptr->dig_P6) / 32768.0;
  v_x2_u32 = v_x2_u32 + v_x1_u32 * ((double)params_ptr->dig_P5) * 2.0;
  v_x2_u32 = (v_x2_u32 / 4.0) + (((double)params_ptr->dig_P4) * 65536.0);
  v_x1_u32 = (((double)params_ptr->dig_P3) * v_x1_u32 * v_x1_u32 / 524288.0 + ((double)params_ptr->dig_P2) * v_x1_u32) / 524288.0;
  v_x1_u32 = (1.0 + v_x1_u32 / 32768.0) * ((double)params_ptr->dig_P1);
  pressure = 1048576.0 - (double)adc_P;
  // Avoid exception caused by division by zero.
  if (v_x1_u32 != 0) pressure = (pressure - (v_x2_u32 / 4096.0)) * 6250.0 / v_x1_u32;
  else return 0;
  v_x1_u32 = ((double)params_ptr->dig_P9) * pressure * pressure / 2147483648.0;
  v_x2_u32 = pressure * ((double)params_ptr->dig_P8) / 32768.0;
  pressure = pressure + (v_x1_u32 + v_x2_u32 + ((double)params_ptr->dig_P7)) / 16.0;
  
  return pressure;
}

static void bme280_extract_long_data(unsigned char *buffer, BME280_S32_t *data_ptr)
{
	unsigned int data_xlsb = 0;
	unsigned int data_lsb = 0;
	unsigned int data_msb = 0;

	data_msb = (unsigned int)buffer[0] << BME280_UPPER_WORD_MASK;
	data_lsb = (unsigned int)buffer[1] << BME280_MIDDLE_WORD_MASK;
	data_xlsb = (unsigned int)buffer[2] >> pressure_temperature_xlsb_mask;
	*data_ptr = data_msb | data_lsb | data_xlsb;
}

//Read all the data from the chip
static Error_Returns bme280_read_data(uint32_t address, BME280_S32_t *adc_T_ptr, BME280_S32_t *adc_P_ptr)
{
	Error_Returns to_return = RPi_NotInitialized;
    unsigned int data_lsb = 0;
    unsigned int data_msb = 0;
	
	unsigned char buffer[BME280_DATA_REGISTER_SIZE];
	unsigned int index = 0;
	//unsigned int status_attempts = 0;
	
	if (bme280_ready)
	{
		do
		{
			for(index = 0; index < BME280_DATA_REGISTER_SIZE; index++) buffer[index] = 0;
			
			buffer[0] = BME280_FIRST_DATA_REGISTER;
			to_return = bme280_read(address, buffer, BME280_DATA_REGISTER_SIZE);
			if (to_return != RPi_Success) break;  //No need to continue, just return the error

		   /* Store the parsed register values for pressure data */
			bme280_extract_long_data(&buffer[0], adc_P_ptr);

			/* Store the parsed register values for temperature data */
			bme280_extract_long_data(&buffer[3], adc_T_ptr);

		} while(0);
	}
	
	return to_return;
}

/* Assumes the I2C bus has been intialized and the id has been vetted
   to be within an acceptable range.
*/
Error_Returns bme280_init(uint32_t id, uint32_t address)
{	
	Error_Returns to_return = RPi_Success;
	unsigned char buffer[BME280_TRIM_PARAMETER_BYTES];
	unsigned int index = 0;
	
	Compensation_Parameters *params_ptr = &bme280_compensation_params[id];
	params_ptr->address = address;
	
	do
	{	
		for(index = 0; index < BME280_TRIM_PARAMETER_BYTES; index++) buffer[index] = 0;
		
		buffer[0] = BME280_CHIP_RPi_REGISTER;
		to_return = bme280_read(address, buffer, 1);
		if (to_return != RPi_Success)
			{
			printf("bme280_init():  Error reading chip ID read was %u\n", to_return);
			break;  //No need to continue just return the failure
			}
		if (buffer[0] != BME280_CHIP_ID)
		{
			printf("bme280_init():  Error chip ID read was 0x%x\n", buffer[0]);
		}
		
		//Read then unpack the compensation parameters stored in the chip
		for(index = 0; index < BME280_TRIM_PARAMETER_BYTES; index++) buffer[index] = 0;
		buffer[0] = BME280_FIRST_TRIM_PARAMETER;
		to_return = bme280_read(address, buffer, BME280_TRIM_PARAMETER_BYTES);
		if (to_return != RPi_Success) break;  //No need to continue just return the failure
		
		index = 0;

		params_ptr->dig_T1 = buffer[index++];
		params_ptr->dig_T1 |= buffer[index++]<<8;
		params_ptr->dig_T2 = buffer[index++];
		params_ptr->dig_T2 |= buffer[index++]<<8;
		params_ptr->dig_T3 = buffer[index++];
		params_ptr->dig_T3 |= buffer[index++]<<8;

		params_ptr->dig_P1 = buffer[index++];
		params_ptr->dig_P1 |= buffer[index++]<<8;
		params_ptr->dig_P2 = buffer[index++];
		params_ptr->dig_P2 |= buffer[index++]<<8;
		params_ptr->dig_P3 = buffer[index++];
		params_ptr->dig_P3 |= buffer[index++]<<8;
		params_ptr->dig_P4 = buffer[index++];
		params_ptr->dig_P4 |= buffer[index++]<<8;
		params_ptr->dig_P5 = buffer[index++];
		params_ptr->dig_P5 |= buffer[index++]<<8;
		params_ptr->dig_P6 = buffer[index++];
		params_ptr->dig_P6 |= buffer[index++]<<8;
		params_ptr->dig_P7 = buffer[index++];
		params_ptr->dig_P7 |= buffer[index++]<<8;
		params_ptr->dig_P8 = buffer[index++];
		params_ptr->dig_P8 |= buffer[index++]<<8;
		params_ptr->dig_P9 = buffer[index++];
		params_ptr->dig_P9 |= buffer[index++]<<8;
		
		for(index = 0; index < BME280_TRIM_PARAMETER_BYTES; index++) buffer[index] = 0;
		buffer[0] = BME280_SECOND_TRIM_PARAMETER;
		to_return = bme280_read(address, buffer, 1);
		if (to_return != RPi_Success) break;  //No need to continue just return the failure
		
		params_ptr->dig_H1 = buffer[0] & 0xFF;
		
		for(index = 0; index < BME280_TRIM_PARAMETER_BYTES; index++) buffer[index] = 0;
		buffer[0] = BME280_THIRD_TRIM_PARAMETER;
		to_return = bme280_read(address, buffer, 7);
		if (to_return != RPi_Success) break;  //No need to continue just return the failure
		
		index = 0;
		
		params_ptr->dig_H2 = buffer[index++] & 0xFF;
		params_ptr->dig_H2|= buffer[index++]<<8;
		params_ptr->dig_H3 = buffer[index++];
		
		params_ptr->dig_H4 = buffer[index++] <<4;
		params_ptr->dig_H4 |= buffer[index] & 0x0F;
		params_ptr->dig_H5 = (buffer[index++] >> 4) & 0x0F;
		params_ptr->dig_H5 |= buffer[index++]<<4;
		params_ptr->dig_H6 = buffer[index++] & 0xFF;

		//Configure the chip appropiately to provide pressure and temperature
		//at a rate appropriate for a Kalman filter
		
		buffer[0] = BME280_CTRL_MEASURE_REGISTER;
		buffer[1] = BME280_SLEEP_MODE; //Need to set up config in sleep mode
		to_return = bme280_write(address, buffer, BME280_CTRL_REGISTER_WRITE_SIZE);
		if (to_return != RPi_Success) break; //Don't continue just return

		buffer[0] = BME280_CTRL_CONFIG_REGISTER;
		buffer[1] = BME280_NO_IIR_16_500MS_STANDBY; //4 wire SPI, IIR 16, .5 ms standby time
		to_return = bme280_write(address, buffer, BME280_CTRL_REGISTER_WRITE_SIZE);
		if (to_return != RPi_Success) break; //Don't continue just return

		buffer[0] = BME280_CTRL_HUMIDITY_REGISTER;
		buffer[1] = BME280_HUMIDITY_OFF;  //No humidity measurements;
		to_return = bme280_write(address, buffer, BME280_CTRL_REGISTER_WRITE_SIZE);
		if (to_return != RPi_Success) break; //Don't continue just return

		buffer[0] = BME280_CTRL_MEASURE_REGISTER;
		//Pressure oversample x16, temp oversample x2, normal mode
		buffer[1] = BME280_PRESS1X_TEMP_1X;
		to_return = bme280_write(address, buffer, BME280_CTRL_REGISTER_WRITE_SIZE);
		if (to_return != RPi_Success) break; //Don't continue just return

		pressure_temperature_xlsb_mask = BME280_IIR_ENABLED_MASK;
		bme280_ready = 1;

		sleep_ms(TIME_DELAY); 				 
	} while(0);
	
	return to_return;
}

Error_Returns bme280_reset(uint32_t id)
{
	Error_Returns to_return = RPi_NotInitialized;
	unsigned char buffer[BME280_CTRL_REGISTER_WRITE_SIZE];
	buffer[0] = BME280_CHIP_RESET_REGISTER;
	buffer[1] = BME280_CHIP_RESET_WORD;
	if (bme280_ready)
	{
		to_return = bme280_write(bme280_compensation_params[id].address, buffer, BME280_CTRL_REGISTER_WRITE_SIZE);
		sleep_ms(TIME_DELAY);  //Delay to allow reset
	}
	return to_return;
}

Error_Returns bme280_get_current_pressure(uint32_t id, double *pressure_ptr)
{
	Error_Returns to_return = RPi_Success;
	BME280_S32_t adc_P = 0;
	BME280_S32_t adc_T = 0;

	do
	{
		to_return = bme280_read_data(bme280_compensation_params[id].address, &adc_T, &adc_P);
		if (to_return != RPi_Success) break;  //No need to continue, just return the error
		compensateTemperature(id, adc_T);	
		*pressure_ptr = compensatePressure(id, adc_P);		
	}  while(0);
	return to_return;
}
