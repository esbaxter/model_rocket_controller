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

File:  bme280.h

Interface into the support software for the BME 280 humidity, temperature
and barometric pressure chip.

*/

#pragma once
#include "hardware/i2c.h"

#include "common.h"
#include "barometer.h"
#include "thermometer.h"

typedef int BME280_S32_t;
typedef unsigned int BME280_U32_t;
typedef long long signed int BME280_S64_t;

Error_Returns bme280_init(uint32_t *id, i2c_inst_t *i2c, uint32_t address);

Error_Returns bme280_reset(uint32_t id);

Error_Returns bme280_get_current_pressure(uint32_t id, uint32_t *pressure_ptr);

Error_Returns bme280_get_current_temperature(uint32_t id, int32_t *temperature_ptr);
