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

File:  mod_roc.c

*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"

#include "common.h"
#include "barometer.h"

#define DESIRED_I2C_BAUD_RATE 100 * 1000

uint init_i2c_bus()
{
    uint baud_rate = i2c_init(i2c_default, DESIRED_I2C_BAUD_RATE);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
	//Set pullups to pull the I2C bus high when idle
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
	return baud_rate;
}

int main() {
	Error_Returns status = RPi_Success;
    stdio_init_all();
	sleep_ms(500); //Let the USB bus get set up

    
	printf("Modroc 1.0 here!\n");
	
	do
	{
		printf("I2C baud rate: %u\n", init_i2c_bus());
		// configure BME280
		status = barometer_init(0, barometer_kalman_filter_mode);
		if (status != RPi_Success)
		{
			printf("barometer_init failed: %u\n", status);
			break;
		}

		while (1) 
		{
			status = barometer_print_compensated_values(0);
			if (status != RPi_Success)
			{
				printf("barometer_print_compensated_values failed: %u\n", status);
				break;
			}	
			printf("\n");
			// poll every 500ms
			sleep_ms(500);
		}
	} while(0);
    return 0;
}
