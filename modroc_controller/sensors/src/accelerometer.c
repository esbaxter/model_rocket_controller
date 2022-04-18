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

File:  accelerometer.c

Interface into the support software for a variety of humidity, temperature
and barometric pressure chips.

*/

#include "accelerometer.h"
#include "icm20948.h"
#include "pico/stdlib.h"

typedef struct Accelerometer_Interface_Struct
{
Error_Returns (*chip_init)(uint32_t *id, i2c_inst_t *i2c, uint32_t address);
Error_Returns (*chip_reset)(uint32_t id);
uint32_t chip_id;
} Accelerometer_Interface;

static uint32_t number_accelerometers_initialized = 0;

static Accelerometer_Interface accelerometer_chip[ACCELEROMETER_NUMBER_SUPPORTED_DEVICES];

/* Current implementation only supports the BME_280 barometer chip, this could
   either become a runtime initialization if a variety of different barometers
   are attached or could be a compile time assignment if all attached 
   barometers are of the same type.
*/
Error_Returns accelerometer_init(uint32_t *id, i2c_inst_t *i2c, uint32_t address)
{
	Error_Returns to_return = RPi_NotInitialized;
	do
	{
		if (number_accelerometers_initialized >= ACCELEROMETER_NUMBER_SUPPORTED_DEVICES)
		{
			break;
		}
		accelerometer_chip[number_accelerometers_initialized].chip_init = icm20948_init;
		accelerometer_chip[number_accelerometers_initialized].chip_reset = icm20948_reset;

		to_return = accelerometer_chip[number_accelerometers_initialized].chip_init(&accelerometer_chip[number_accelerometers_initialized].chip_id, i2c, address);

		/* If the chip initialization isn't successfull then there is no
		   need to bother the client with the details, just return that
		   this accelerometer isn't initialized.
		*/
		if (to_return != RPi_Success)
		{
			to_return = RPi_NotInitialized;
			break;
		}
		
		*id = number_accelerometers_initialized++;
	} while(0);
	return to_return;
}


Error_Returns accelerometer_reset(uint32_t id)
{
	Error_Returns to_return = RPi_NotInitialized;
	if (id < number_accelerometers_initialized)
	{
		to_return = accelerometer_chip[id].chip_reset(accelerometer_chip[id].chip_id);
	}
	return to_return;
}

