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

File:  output_task.c

*/

#include <stdio.h>
#include "pico/stdlib.h"

#include "common.h"
#include "output_task.h"

void output_task() {

	do
	{
		while (1) 
		{
			Intertask_Param_Message_t param_entry;
			Log_Message_t log_entry;

			if (message_log_get_params(&param_entry))
			{
				switch (param_entry.message_type)
				{
					case message_log_ascent_parameters:
						printf("%u: altitude: %d z accel: %hu z velocity %hu\n", 
						   param_entry.time_stamp, param_entry.message.log_ascent_parameters.altitude, 
						   param_entry.message.log_ascent_parameters.z_acceleration,
						   param_entry.message.log_ascent_parameters.z_velocity);				
						break;
					
					case message_log_descent_parameters:
						printf("%u: altitude %d temperature: %f\n", 
						   param_entry.time_stamp, param_entry.message.log_descent_parameters.altitude, param_entry.message.log_descent_parameters.temperature);
						break;
					default:
						message_send_log("output_task:  Rx'd unknown message %u\n", param_entry.message_type);
					break;
				}
			}
			
			if (message_get_log(&log_entry))
			{
				printf("%u: %s", log_entry.time_stamp, log_entry.log_message);
			}
		}
	} while(0);

}
