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

File:  accelerometer.h

Interface into the support software for a variety of accelerometers.

*/

#pragma once
#include "hardware/spi.h"

#include "common.h"

#define ACCELEROMETER_NUMBER_SUPPORTED_DEVICES 1

/*  Initializes a accelerometer with the given address on the specified I2C bus, 
	if the maximum number of accelerometers are exceeded or the accelerometer chip fails 
	to initialize returns RPi_NotInitialized, otherwise RPi_Success is returned 
	along with a valid ID.
*/
	
Error_Returns accelerometer_init(uint32_t *id, spi_inst_t *i2c, uint32_t chip_select);

Error_Returns accelerometer_reset(uint32_t id);
