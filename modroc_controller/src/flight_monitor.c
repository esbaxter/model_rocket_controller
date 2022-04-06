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
#include "flight_monitor.h"
#include "altimeter.h"

#define LOG_FLIGHT_PARAMETERS_TIMER_MS 1000

typedef enum {
	phase_pad_idle,
	phase_ascent,
	phase_descent,
} Flight_Phase;

//Out here for safety's sake
repeating_timer_t timer;
static int32_t current_altitude = 0;
static int32_t previous_altitude = 0;
static Flight_Phase current_flight_phase = phase_pad_idle;

static bool log_ascent_parameters(repeating_timer_t *rt) 
{
	Log_Ascent_Parameters_t entry;
	previous_altitude = current_altitude;
	current_altitude = entry.altitude = altimeter_get_delta();
	entry.z_acceleration = 0;
	entry.z_velocity = 0;
	message_log_ascent_params(&entry);

	return true; // keep repeating	
}

static bool log_descent_parameters(repeating_timer_t *rt) 
{
	Log_Descent_Parameters_t entry;
	previous_altitude = current_altitude;
	current_altitude = entry.altitude = altimeter_get_delta();
	entry.temperature = 0;  //Add thermometer
	message_log_descent_params(&entry);

	return true; // keep repeating	
}

void flight_monitor() {

	do
	{
		Error_Returns status = RPi_Success;

		if (!add_repeating_timer_ms(LOG_FLIGHT_PARAMETERS_TIMER_MS, log_ascent_parameters, NULL, &timer)) {
			message_send_log("input_task(): Failed to add log_ascent_parameters timer\n");
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
			
			//When available transition from pad_idle to ascent when z acceleration is greater than 1G
			if ((current_flight_phase == phase_pad_idle) && (current_altitude == 1))
			{
				current_flight_phase = phase_ascent;
			}
	
			if ((current_flight_phase == phase_ascent) && ((current_altitude -  previous_altitude) == -1))
			{
				cancel_repeating_timer(&timer);
				if (!add_repeating_timer_ms(LOG_FLIGHT_PARAMETERS_TIMER_MS, log_descent_parameters, NULL, &timer)) 
				{
				message_send_log("input_task(): Failed to add log_descent_parameters timer\n");			
				break;
				}
				current_flight_phase = phase_descent;
			}
		
		}
	} while(0);

}
