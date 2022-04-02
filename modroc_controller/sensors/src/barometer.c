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

File:  barometer.

Interface into the support software for a variety of humidity, temperature
and barometric pressure chips.

*/

#include "barometer.h"
#include "bme280.h"
#include "pico/stdlib.h"

typedef struct Barometer_Interface_Struct
{
Error_Returns (*chip_init)(uint32_t id, uint32_t address);
Error_Returns (*chip_reset)(uint32_t id);
Error_Returns (*chip_get_current_pressure)(uint32_t id, uint32_t *pressure_ptr);
uint32_t address;
} Barometer_Interface;

static uint32_t number_barometers_initialized = 0;

static Barometer_Interface barometer_chip[BAROMETER_NUMBER_SUPPORTED_DEVICES];

/* Current implementation only supports the BME_280 barometer chip, this could
   either become a runtime initialization if a variety of different barometers
   are attached or could be a compile time assignment if all attached 
   barometers are of the same type.
*/
Error_Returns barometer_init(uint32_t *id, uint32_t address)
{
	Error_Returns to_return = RPi_NotInitialized;
	do
	{
		if (number_barometers_initialized >= BAROMETER_NUMBER_SUPPORTED_DEVICES)
		{
			break;
		}
		barometer_chip[number_barometers_initialized].chip_init = bme280_init;
		barometer_chip[number_barometers_initialized].chip_reset = bme280_reset;
		barometer_chip[number_barometers_initialized].chip_get_current_pressure = bme280_get_current_pressure;

		to_return = barometer_chip[number_barometers_initialized].chip_init(number_barometers_initialized, address);

		/* If the chip initialization isn't successfull then there is no
		   need to bother the client with the details, just return that
		   this barometer isn't initialized.
		*/
		if (to_return != RPi_Success)
		{
			to_return = RPi_NotInitialized;
			break;
		}
		
		*id = number_barometers_initialized++;
	} while(0);
	return to_return;
}

Error_Returns barometer_reset(uint32_t id)
{
	Error_Returns to_return = RPi_NotInitialized;
	if (id < number_barometers_initialized)
	{
		to_return = barometer_chip[id].chip_reset(id);
	}
	return to_return;
}

Error_Returns barometer_get_current_pressure(uint32_t id, uint32_t *pressure_ptr)
{
	Error_Returns to_return = RPi_NotInitialized;
	if (id < number_barometers_initialized)
	{
		to_return = barometer_chip[id].chip_get_current_pressure(id, pressure_ptr);
	}
	return to_return;
}
