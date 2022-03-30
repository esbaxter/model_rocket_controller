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

typedef struct Barometer_Interface_Struct
{
Error_Returns (*chip_init)(uint32_t id, Barometer_mode mode);
Error_Returns (*chip_reset)(uint32_t id);
Error_Returns (*chip_print_compensated_values)(uint32_t id);
Error_Returns (*chip_get_current_pressure)(uint32_t id, double *pressure_ptr);
Error_Returns (*chip_get_current_temperature)(uint32_t id, double *temperature_ptr);
Error_Returns (*chip_get_current_temperature_pressure)(uint32_t id, double *temperature_ptr, double *pressure_ptr);
} Barometer_Interface;

static uint32_t barometer_initialized = 0;

static Barometer_Interface barometer_chip[NUMBER_SUPPORTED_DEVICES];

Error_Returns barometer_init(uint32_t id, Barometer_mode mode)
{
	Error_Returns to_return = RPi_NotInitialized;
	do
	{
		if (id < NUMBER_SUPPORTED_DEVICES)
		{
			to_return = RPi_InvalidParam;
			break;
		}
		barometer_chip[id].chip_init = bme280_init;
		barometer_chip[id].chip_reset = bme280_reset;
		barometer_chip[id].chip_print_compensated_values = bme280_print_compensated_values;
		barometer_chip[id].chip_get_current_pressure = bme280_get_current_pressure;
		barometer_chip[id].chip_get_current_temperature = bme280_get_current_temperature;
		barometer_chip[id].chip_get_current_temperature_pressure = bme280_get_current_temperature_pressure;
		barometer_initialized = 1;
		to_return = barometer_chip[id].chip_init(id, mode);
	} while(0);
	return to_return;
}

Error_Returns barometer_reset(uint32_t id)
{
	Error_Returns to_return = RPi_NotInitialized;
	if (barometer_initialized && (id < NUMBER_SUPPORTED_DEVICES))
	{
		to_return = barometer_chip[id].chip_reset(id);
	}
	return to_return;
}

Error_Returns barometer_print_compensated_values(uint32_t id)
{
	Error_Returns to_return = RPi_NotInitialized;
	if (barometer_initialized && (id < NUMBER_SUPPORTED_DEVICES))
	{
		to_return = barometer_chip[id].chip_print_compensated_values(id);
	}
	return to_return;
}


Error_Returns barometer_get_current_pressure(uint32_t id, double *pressure_ptr)
{
	Error_Returns to_return = RPi_NotInitialized;
	if (barometer_initialized && (id < NUMBER_SUPPORTED_DEVICES))
	{
		to_return = barometer_chip[id].chip_get_current_pressure(id, pressure_ptr);
	}
	return to_return;
}


Error_Returns barometer_get_current_temperature(uint32_t id, double *temperature_ptr)
{
	Error_Returns to_return = RPi_NotInitialized;
	if (barometer_initialized && (id < NUMBER_SUPPORTED_DEVICES))
	{
		to_return = barometer_chip[id].chip_get_current_temperature(id, temperature_ptr);
	}
	return to_return;
}


Error_Returns barometer_get_current_temperature_pressure(uint32_t id, double *temperature_ptr, double *pressure_ptr)
{
	Error_Returns to_return = RPi_NotInitialized;
	if (barometer_initialized && (id < NUMBER_SUPPORTED_DEVICES))
	{
		to_return = barometer_chip[id].chip_get_current_temperature_pressure(id,
			temperature_ptr, pressure_ptr);
	}
	return to_return;
}