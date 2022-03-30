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

File:  barometer.h

Interface into the support software for a variety of humidity, temperature
and barometric pressure chips.

*/

#pragma once
#include "common.h"

#define NUMBER_SUPPORTED_DEVICES 2

typedef enum {
	barometer_temp_pressure_humidity,
	barometer_altitude_mode,
	barometer_kalman_filter_mode
} Barometer_mode;

Error_Returns barometer_init(uint32_t id, Barometer_mode mode);

Error_Returns barometer_reset(uint32_t id);

Error_Returns barometer_print_compensated_values(uint32_t id);

Error_Returns barometer_get_current_pressure(uint32_t id, double *pressure_ptr);

Error_Returns barometer_get_current_temperature(uint32_t id, double *temperature_ptr);

Error_Returns barometer_get_current_temperature_pressure(uint32_t id, double *temperature_ptr, double *pressure_ptr);