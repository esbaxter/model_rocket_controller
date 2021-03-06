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
#include "pico/stdlib.h"

#include "bme280.h"

#define BME280_SUPPORTED_DEVICE_COUNT BAROMETER_NUMBER_SUPPORTED_DEVICES + THERMOMETER_NUMBER_SUPPORTED_DEVICES

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
	i2c_inst_t *i2c;
} Compensation_Parameters;

static Compensation_Parameters bme280_compensation_params[BME280_SUPPORTED_DEVICE_COUNT];

static uint32_t number_bme280_initialized = 0;

static uint32_t pressure_temperature_xlsb_mask = 0;

//Communication routine with the BME 280
static Error_Returns bme280_write(Compensation_Parameters *params_ptr, unsigned char *buffer, unsigned int tx_bytes)
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
static Error_Returns bme280_read(Compensation_Parameters *params_ptr, unsigned char *buffer, unsigned int rx_bytes)
{
	Error_Returns to_return = RPi_Success;
	to_return = bme280_write(params_ptr, buffer, 1);
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

//Taken straight from the Bosch manual.
static int32_t compensate_temperature(uint32_t id, uint32_t uncompensated_temperature)
{
    int32_t var1;
    int32_t var2;
    int32_t temperature;
    int32_t temperature_min = -4000;
    int32_t temperature_max = 8500;
	Compensation_Parameters *calib_data = &bme280_compensation_params[id];

    var1 = (int32_t)((uncompensated_temperature / 8) - ((int32_t)calib_data->dig_T1 * 2));
    var1 = (var1 * ((int32_t)calib_data->dig_T2)) / 2048;
    var2 = (int32_t)((uncompensated_temperature / 16) - ((int32_t)calib_data->dig_T1));
    var2 = (((var2 * var2) / 4096) * ((int32_t)calib_data->dig_T3)) / 16384;
    calib_data->t_fine = var1 + var2;
    temperature = (calib_data->t_fine * 5 + 128) / 256;

    if (temperature < temperature_min)
    {
        temperature = temperature_min;
    }
    else if (temperature > temperature_max)
    {
        temperature = temperature_max;
    }

    return temperature;
}

// Straight from the Bosch manual.
static uint32_t compensate_pressure(uint32_t id, int32_t adc_P)
{
    int64_t var1;
    int64_t var2;
    int64_t var3;
    int64_t var4;
    uint32_t pressure;
    uint32_t pressure_min = 3000000;
    uint32_t pressure_max = 11000000;
	Compensation_Parameters *calib_data = &bme280_compensation_params[id];
	
    var1 = ((int64_t)calib_data->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib_data->dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib_data->dig_P5) * 131072);
    var2 = var2 + (((int64_t)calib_data->dig_P4) * 34359738368);
    var1 = ((var1 * var1 * (int64_t)calib_data->dig_P3) / 256) + ((var1 * ((int64_t)calib_data->dig_P2) * 4096));
    var3 = ((int64_t)1) * 140737488355328;
    var1 = (var3 + var1) * ((int64_t)calib_data->dig_P1) / 8589934592;

    /* To avoid divide by zero exception */
    if (var1 != 0)
    {
        var4 = 1048576 - adc_P;
        var4 = (((var4 * INT64_C(2147483648)) - var2) * 3125) / var1;
        var1 = (((int64_t)calib_data->dig_P9) * (var4 / 8192) * (var4 / 8192)) / 33554432;
        var2 = (((int64_t)calib_data->dig_P8) * var4) / 524288;
        var4 = ((var4 + var1 + var2) / 256) + (((int64_t)calib_data->dig_P7) * 16);
        pressure = (uint32_t)(((var4 / 2) * 100) / 128);

        if (pressure < pressure_min)
        {
            pressure = pressure_min;
        }
        else if (pressure > pressure_max)
        {
            pressure = pressure_max;
        }
    }
    else
    {
        pressure = pressure_min;
    }

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
static Error_Returns bme280_read_data(Compensation_Parameters *params_ptr, BME280_S32_t *adc_T_ptr, BME280_S32_t *adc_P_ptr)
{
	Error_Returns to_return = RPi_NotInitialized;
    unsigned int data_lsb = 0;
    unsigned int data_msb = 0;
	
	unsigned char buffer[BME280_DATA_REGISTER_SIZE];
	unsigned int index = 0;
	//unsigned int status_attempts = 0;
	
	do
	{
		for(index = 0; index < BME280_DATA_REGISTER_SIZE; index++) buffer[index] = 0;
		
		buffer[0] = BME280_FIRST_DATA_REGISTER;
		to_return = bme280_read(params_ptr, buffer, BME280_DATA_REGISTER_SIZE);
		if (to_return != RPi_Success) break;  //No need to continue, just return the error

	   /* Store the parsed register values for pressure data */
		bme280_extract_long_data(&buffer[0], adc_P_ptr);

		/* Store the parsed register values for temperature data */
		bme280_extract_long_data(&buffer[3], adc_T_ptr);

	} while(0);
	
	return to_return;
}

/* Assumes the I2C bus has been intialized.
*/
Error_Returns bme280_init(uint32_t *id, i2c_inst_t *i2c, uint32_t address)
{	
	Error_Returns to_return = RPi_Success;
	unsigned char buffer[BME280_TRIM_PARAMETER_BYTES];
	unsigned int index = 0;
	

	if (number_bme280_initialized < BME280_SUPPORTED_DEVICE_COUNT)
	{
		do
		{
			Compensation_Parameters *params_ptr = &bme280_compensation_params[number_bme280_initialized];
			params_ptr->address = address;
			params_ptr->i2c = i2c;
			
			for(index = 0; index < BME280_TRIM_PARAMETER_BYTES; index++) buffer[index] = 0;
			
			buffer[0] = BME280_CHIP_RPi_REGISTER;
			to_return = bme280_read(params_ptr, buffer, 1);
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
			to_return = bme280_read(params_ptr, buffer, BME280_TRIM_PARAMETER_BYTES);
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
			to_return = bme280_read(params_ptr, buffer, 1);
			if (to_return != RPi_Success) break;  //No need to continue just return the failure
			
			params_ptr->dig_H1 = buffer[0] & 0xFF;
			
			for(index = 0; index < BME280_TRIM_PARAMETER_BYTES; index++) buffer[index] = 0;
			buffer[0] = BME280_THIRD_TRIM_PARAMETER;
			to_return = bme280_read(params_ptr, buffer, 7);
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
			to_return = bme280_write(params_ptr, buffer, BME280_CTRL_REGISTER_WRITE_SIZE);
			if (to_return != RPi_Success) break; //Don't continue just return

			buffer[0] = BME280_CTRL_CONFIG_REGISTER;
			buffer[1] = BME280_NO_IIR_16_500MS_STANDBY; //4 wire SPI, IIR 16, .5 ms standby time
			to_return = bme280_write(params_ptr, buffer, BME280_CTRL_REGISTER_WRITE_SIZE);
			if (to_return != RPi_Success) break; //Don't continue just return

			buffer[0] = BME280_CTRL_HUMIDITY_REGISTER;
			buffer[1] = BME280_HUMIDITY_OFF;  //No humidity measurements;
			to_return = bme280_write(params_ptr, buffer, BME280_CTRL_REGISTER_WRITE_SIZE);
			if (to_return != RPi_Success) break; //Don't continue just return

			buffer[0] = BME280_CTRL_MEASURE_REGISTER;
			//Pressure oversample x16, temp oversample x2, normal mode
			buffer[1] = BME280_PRESS1X_TEMP_1X;
			to_return = bme280_write(params_ptr, buffer, BME280_CTRL_REGISTER_WRITE_SIZE);
			if (to_return != RPi_Success) break; //Don't continue just return

			pressure_temperature_xlsb_mask = BME280_IIR_ENABLED_MASK;

			*id = number_bme280_initialized++;
			sleep_ms(TIME_DELAY); 				 
		} while(0);
	}
	
	return to_return;
}

Error_Returns bme280_reset(uint32_t id)
{
	Error_Returns to_return = RPi_NotInitialized;
	if (id < number_bme280_initialized)
	{
		Compensation_Parameters *params_ptr = &bme280_compensation_params[id];
	
		unsigned char buffer[BME280_CTRL_REGISTER_WRITE_SIZE];
		buffer[0] = BME280_CHIP_RESET_REGISTER;
		buffer[1] = BME280_CHIP_RESET_WORD;
		to_return = bme280_write(params_ptr, buffer, BME280_CTRL_REGISTER_WRITE_SIZE);
		sleep_ms(TIME_DELAY);  //Delay to allow reset
	}
	return to_return;
}

Error_Returns bme280_get_current_pressure(uint32_t id, uint32_t *pressure_ptr)
{
	Error_Returns to_return = RPi_NotInitialized;
	if (id < number_bme280_initialized)
	{
		Compensation_Parameters *params_ptr = &bme280_compensation_params[id];
	
		BME280_S32_t adc_P = 0;
		BME280_S32_t adc_T = 0;

		do
		{
			to_return = bme280_read_data(params_ptr, &adc_T, &adc_P);
			if (to_return != RPi_Success) break;  //No need to continue, just return the error
			compensate_temperature(id, adc_T);	
			*pressure_ptr = compensate_pressure(id, adc_P);		
		}  while(0);
	}
	return to_return;
}

Error_Returns bme280_get_current_temperature(uint32_t id, int32_t *temperature_ptr)
{
	Error_Returns to_return = RPi_NotInitialized;
	if (id < number_bme280_initialized)
	{
		Compensation_Parameters *params_ptr = &bme280_compensation_params[id];
		
		BME280_S32_t adc_P = 0;
		BME280_S32_t adc_T = 0;

		do
		{
			to_return = bme280_read_data(params_ptr, &adc_T, &adc_P);
			if (to_return != RPi_Success) break;  //No need to continue, just return the error
			*temperature_ptr = compensate_temperature(id, adc_T);		
		}  while(0);
	}
	return to_return;
}
