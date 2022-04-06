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

File:  mod_roc.c

*/

#include <stdio.h>

#include "common.h"
#include "hardware_platform.h"
#include "message.h"
#include "output_task.h"
#include "flight_monitor.h"

int main() {
	Error_Returns status = RPi_Success;
    stdio_init_all();
	sleep_ms(500); //Let the USB bus get set up
	
	do
	{
		message_init();		
		status = configure_hardware_platform();
		
		//Launch the task to handle logging, etc.
		multicore_launch_core1(output_task);
		
		if (status != RPi_Success)
		{
			message_send_log("configure_hardware_platform failed: %u\n", status);
			while (1)
			{
				sleep_ms(500);
			}
		}
		else
		{

			//Dive into the code that reads inputs and does
			//the calculations.  If it returns something bad
			//happened.
			flight_monitor();
		}		
	} while(0);
    return 0;
}
