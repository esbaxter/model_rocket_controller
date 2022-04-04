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

File:  intertask_message.c

*/

#include "common.h"
#include "intertask_message.h"

//Publicly shared between input task and output task.
queue_t output_task_queue;

void intertask_message_init()
{
	queue_init(&output_task_queue, sizeof(Intertask_Message_t), MAX_NUMBER_MESSAGES);
}

void intertask_message_send_log(char *format, ...)
{
	Intertask_Message_t entry;
	va_list args;
	va_start (args, format);
	vsprintf (entry.message.log_message.log_message, format, args);	
	
	entry.message_type = message_log_message;
	entry.message.log_parameters.time_stamp = GET_TIME_STAMP;

	queue_add_blocking(&output_task_queue, &entry);
}