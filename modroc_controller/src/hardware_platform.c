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

File:  hardware_platform.c

This file aggregates all the vagaries of initializing the Pico along with the
necessary external chips.  This also initializes all modules the depend on the
hardware (e.g. altimeter).

*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"

#include "common.h"
#include "message.h"
#include "hardware_platform.h"
#include "barometer.h"
#include "altimeter.h"
#include "thermometer.h"

#define DESIRED_I2C_BAUD_RATE 100 * 1000
#define BAROMETER_ADDRESS 0x76
#define BAROMETER_COUNT 1

#define THERMOMETER_ADDRESS 0x76

static uint32_t barometer_id[BAROMETER_COUNT] = {0xffffffff};

static Error_Returns configure_busses()
{
	Error_Returns to_return = RPi_NotInitialized;
    do
	{
		uint baud_rate = i2c_init(i2c0, DESIRED_I2C_BAUD_RATE);
		if (baud_rate != DESIRED_I2C_BAUD_RATE)
		{
			printf("configure_busses:  failed to configure i2c\n");
			break;
		}
		
		gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
		gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
		//Set pullups to pull the I2C bus high when idle
		gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
		gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
		to_return = RPi_Success;
	} while(0);
	return to_return;
}

static Error_Returns configure_altimeter()
{
	Error_Returns to_return = RPi_NotInitialized;
	
	do
	{
		//Current design has all barometeric chips on i2c0
		to_return = barometer_init(&barometer_id[0], i2c0, BAROMETER_ADDRESS);
		if (to_return != RPi_Success)
		{
			message_send_log("configure_altimeter():  barometer_init failed: %u\n", to_return);
			break;
		}
		
		to_return = altimeter_initialize(&barometer_id[0], BAROMETER_COUNT);
		if (to_return != RPi_Success)
		{
			message_send_log("configure_altimeter():  altimeter_initialize failed: %u\n", to_return);
			break;
		}		
	} while(0);
	return to_return;
}

static Error_Returns configure_thermometer()
{
	return thermometer_init(i2c0, THERMOMETER_ADDRESS);
}

Error_Returns configure_hardware_platform()
{
	Error_Returns to_return = RPi_NotInitialized;

	do
	{
		to_return = configure_busses();
		if (to_return != RPi_Success)
		{
			message_send_log("configure_hardware_platform:  configure_busses failed\n");
			break;
		}

		to_return = configure_altimeter();
		
		if (to_return != RPi_Success)
		{
			message_send_log("configure_hardware_platform:  configure_altimeter failed\n");
			break;
		}

		to_return = configure_thermometer();
		
		if (to_return != RPi_Success)
		{
			message_send_log("configure_hardware_platform:  configure_thermometer failed\n");
			break;
		}
	}
	while(0);
	return to_return;
}