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

File:  input_task.c

*/


#include <stdio.h>
#include "pico/stdlib.h"

#include "common.h"
#include "input_task.h"
#include "altimeter.h"

#define LOG_FLIGHT_PARAMETERS_TIMER_MS 1000

//Out here for safety's sake
repeating_timer_t timer;

bool log_flight_parameters(repeating_timer_t *rt) 
{
	Log_Ascent_Parameters_t entry;
	entry.altitude = altimeter_get_delta();
	entry.z_acceleration = 0;
	entry.z_velocity = 0;
	message_log_ascent_params(&entry);

	return true; // keep repeating	
}

void input_task() {

	do
	{
		Error_Returns status = RPi_Success;

		if (!add_repeating_timer_ms(LOG_FLIGHT_PARAMETERS_TIMER_MS, log_flight_parameters, NULL, &timer)) {
			message_send_log("input_task(): Failed to add timer\n");
			break;;
		}
		
		while (1) 
		{
			status = altimeter_update_altitude();
			if (status != RPi_Success)
			{
				message_send_log("input_task(): altimeter_update_altitude failed: %u\n", status);
				sleep_ms(500); //Let the message get sent...
				break;
			}
		}
	} while(0);

}
