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

#define ICM20948_SUPPORTED_DEVICE_COUNT ACCELEROMETER_NUMBER_SUPPORTED_DEVICES
#define ICM20948_REGISTER_RW_SIZE	2

#define ICM20948_WHO_AM_I_REGISTER	0x00

#define ICM20948_WHO_AM_I_VALUE		0xea

typedef struct ICM20948_Params {
	uint32_t address;
	i2c_inst_t *i2c;
} ICM20948_Parameters;

static ICM20948_Parameters icm20948_params[ICM20948_SUPPORTED_DEVICE_COUNT];
static uint32_t number_icm20948_initialized = 0;

//Communication routine with the ICM 20948
static Error_Returns icm20948_write(ICM20948_Parameters *params_ptr, unsigned char *buffer, unsigned int tx_bytes)
{	
	Error_Returns to_return = RPi_Success;
	if (i2c_write_blocking(params_ptr->i2c, params_ptr->address, buffer, tx_bytes, false) ==
		PICO_ERROR_GENERIC)
	{
		to_return = I2CS_Data_Loss;
	}
	return to_return;
}


//Communication routine with the BME 280
static Error_Returns icm20948_read(ICM20948_Parameters *params_ptr, unsigned char *buffer, unsigned int rx_bytes)
{
	Error_Returns to_return = RPi_Success;
	to_return = icm20948_write(params_ptr, buffer, 1);
	if (to_return == RPi_Success)
	{
		if (i2c_read_blocking(params_ptr->i2c, params_ptr->address, buffer, rx_bytes, false) ==
			PICO_ERROR_GENERIC)
		{
			to_return = I2CS_Data_Loss;
		}
	}
	return to_return;
}

/* Assumes the I2C bus has been intialized.
*/
Error_Returns icm20948_init(uint32_t *id, i2c_inst_t *i2c, uint32_t address)
{	
	Error_Returns to_return = RPi_Success;
	unsigned char buffer[ICM20948_REGISTER_RW_SIZE];

	if (number_icm20948_initialized < ICM20948_SUPPORTED_DEVICE_COUNT)
	{
		do
		{
			ICM20948_Parameters *params_ptr = &icm20948_params[number_icm20948_initialized];
			params_ptr->address = address;
			params_ptr->i2c = i2c;
			
			buffer[0] = ICM20948_WHO_AM_I_REGISTER;
			to_return = icm20948_read(params_ptr, buffer, 1);
			if (to_return != RPi_Success)
				{
				printf("icm20948_init():  Error reading chip ID read was %u\n", to_return);
				break;  //No need to continue just return the failure
				}
			if (buffer[0] != ICM20948_WHO_AM_I_VALUE)
			{
				printf("icm20948_init():  Error chip ID read was 0x%x\n", buffer[0]);
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

