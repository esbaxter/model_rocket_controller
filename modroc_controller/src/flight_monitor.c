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

#define DEFAULT_ASCENT_TIMER_MS 1000
#define DEFAULT_DESCENT_TIMER_MS 1000

typedef struct Critical_Flight_Params_S
{
	int32_t current_altitude;
	int32_t previous_altitude;
} Critical_Flight_Params_t;

typedef enum {
	phase_initial,
	phase_pad_idle,
	phase_ascent,
	phase_descent,
	phase_ground_idle
} Flight_Phase;

static int32_t ascent_timer_interval = DEFAULT_ASCENT_TIMER_MS;
static int32_t descent_timer_interval = DEFAULT_DESCENT_TIMER_MS;

//Handler for logging during descent, called from the
//repeating timer code provided in the SDK.
static bool log_ascent_parameters(repeating_timer_t *rt) 
{
	Log_Ascent_Parameters_t entry;
	Critical_Flight_Params_t *critical_flight_params = (Critical_Flight_Params_t *) rt->user_data;
	
	critical_flight_params->previous_altitude = critical_flight_params->current_altitude;
	critical_flight_params->current_altitude = entry.altitude = altimeter_get_delta();
	entry.z_acceleration = 0;
	entry.z_velocity = 0;
	message_log_ascent_params(&entry);

	return true; // keep repeating	
}

//Handler for logging during descent, called from the
//repeating timer code provided in the SDK.
static bool log_descent_parameters(repeating_timer_t *rt) 
{
	Log_Descent_Parameters_t entry;
	Critical_Flight_Params_t *critical_flight_params = (Critical_Flight_Params_t *) rt->user_data;
	
	critical_flight_params->previous_altitude = critical_flight_params->current_altitude;
	critical_flight_params->current_altitude = entry.altitude = altimeter_get_delta();
	entry.temperature = 0;  //Add thermometer
	message_log_descent_params(&entry);

	return true; // keep repeating	
}

//Finite state machine to handle flight phases, see
//design document for the graphic representation.  Decided
//on switch statement design rather than 2D table since this
//may get fairly complex and didn't want to have a large sparse
//table taking up memory.
static Error_Returns flight_state_machine()
{
	static Flight_Phase current_flight_phase = phase_initial;
	static repeating_timer_t timer;
	static Critical_Flight_Params_t critical_flight_params;
	Error_Returns to_return = RPi_Success;
	
	switch(current_flight_phase)
	{
		case phase_initial:  //Initialize state and transition to pad idle
		{
			critical_flight_params.current_altitude = 0;
			critical_flight_params.previous_altitude = 0;
			current_flight_phase = phase_pad_idle;
			message_send_log("Pad idle\n");
			break;
		}
		
		case phase_pad_idle:  //Check to see if we have lifted off, if so start logging.
		{			
			//When available transition from pad_idle to ascent when z acceleration is greater than 1G
			if (altimeter_get_delta() >= 1)
			{
				if (!add_repeating_timer_ms(ascent_timer_interval, log_ascent_parameters, &critical_flight_params, &timer)) 
				{
					message_send_log("flight_state_machine(): Failed to add log_ascent_parameters timer\n");
					to_return = RPi_OperationFailed;
				}
				else
				{
					message_send_log("Liftoff!\n");
					current_flight_phase = phase_ascent;
				}
			}
			break;
		}
		
		case phase_ascent:  //Check for apogee, if found transition to descent
		{
			if ((critical_flight_params.current_altitude - critical_flight_params.previous_altitude) == -1)
			{
				cancel_repeating_timer(&timer);
				if (!add_repeating_timer_ms(descent_timer_interval, log_descent_parameters, &critical_flight_params, &timer)) 
				{
					message_send_log("flight_state_machine(): Failed to add log_descent_parameters timer\n");	
					to_return = RPi_OperationFailed;					
				}
				else
				{
					message_send_log("Apogee!\n");
					current_flight_phase = phase_descent;
				}
			}
			break;
		}
		
		case phase_descent:  //Check for landing
		{
			if (critical_flight_params.current_altitude == 0)
			{
				cancel_repeating_timer(&timer);
				message_send_log("Landed!\n");
				current_flight_phase = phase_ground_idle;
			}
			break;
		}
		
		case phase_ground_idle:  //Not sure what we need to do in this state.
		{
			break;
		}

		default:
		{
			message_send_log("flight_state_machine(): Uknown FSM state %u\n", current_flight_phase);
			break;
		}
	}
	return to_return;
}

//Basic loop to handle monitoring and control of the flight
void flight_monitor() 
{
	do
	{
		Error_Returns status = RPi_Success;
		//Set a go indicator here
		while (1) 
		{
			status = flight_state_machine();
			if (status != RPi_Success)
			{
				message_send_log("flight_monitor(): flight_state_machine failed: %u\n", status);
				sleep_ms(500); //Let the message get sent...
				break;
			}	

			status = altimeter_update_altitude();
			if (status != RPi_Success)
			{
				message_send_log("flight_monitor(): altimeter_update_altitude failed: %u\n", status);
				sleep_ms(500); //Let the message get sent...
				break;
			}
		}
	} while(0);
	//Set a failure indicator here
}

void flight_monitor_set_timer_intervals(int32_t ascent_timer_interval_ms,
	int32_t descent_timer_interval_ms)
{
	ascent_timer_interval = ascent_timer_interval_ms;
	descent_timer_interval = descent_timer_interval_ms;
}
