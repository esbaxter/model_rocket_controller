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

File:  thermometer.c

Interface into the support software for a variety of humidity, temperature
and barometric pressure chips.

*/

#include "thermometer.h"
#include "bme280.h"
#include "pico/stdlib.h"

typedef struct Thermometer_Interface_Struct
{
Error_Returns (*chip_init)(uint32_t *id, i2c_inst_t *i2c, uint32_t address);
Error_Returns (*chip_reset)(uint32_t id);
Error_Returns (*chip_get_current_temperature)(uint32_t id, int32_t *temperature_ptr);
uint32_t chip_id;
} Thermometer_Interface;

static Thermometer_Interface thermometer_chip;
static unsigned char thermometer_ready = 0;

/* Current implementation only supports the BME_280 barometer chip, this could
   either become a runtime initialization if a variety of different thermometers
   are attached or could be a compile time assignment if all attached 
   thermometers are of the same type.
*/
Error_Returns thermometer_init(i2c_inst_t *i2c, uint32_t address)
{
	Error_Returns to_return = RPi_NotInitialized;
	if (!thermometer_ready)
	{
		do
		{
			thermometer_chip.chip_init = bme280_init;
			thermometer_chip.chip_reset = bme280_reset;
			thermometer_chip.chip_get_current_temperature = bme280_get_current_temperature;

			to_return = thermometer_chip.chip_init(&thermometer_chip.chip_id, i2c, address);

			/* If the chip initialization isn't successfull then there is no
			   need to bother the client with the details, just return that
			   this barometer isn't initialized.
			*/
			if (to_return != RPi_Success)
			{
				to_return = RPi_NotInitialized;
				break;
			}
			
			thermometer_ready++;
		} while(0);
	}
	return to_return;
}

Error_Returns thermometer_reset(uint32_t id)
{
	Error_Returns to_return = RPi_NotInitialized;
	if (thermometer_ready)
	{
		to_return = thermometer_chip.chip_reset(thermometer_chip.chip_id);
	}
	return to_return;
}

double thermometer_get_current_temperature()
{
	double to_return = 0.0;
	int32_t temperature_int;
	if (thermometer_ready)
	{
		to_return = thermometer_chip.chip_get_current_temperature(thermometer_chip.chip_id, &temperature_int);
		to_return = (float)temperature_int/100.0;
	}
	return to_return;
}
