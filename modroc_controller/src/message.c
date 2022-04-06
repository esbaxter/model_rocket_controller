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
#include "message.h"

//Queue for sending messages.
static queue_t log_params_queue;
static queue_t log_queue;

void message_init()
{
	queue_init(&log_params_queue, sizeof(Intertask_Param_Message_t), MAX_NUMBER_MESSAGES);
	queue_init(&log_queue, sizeof(Log_Message_t), MAX_NUMBER_MESSAGES);
}

void message_log_ascent_params(Log_Ascent_Parameters_t *log_ascent_parameters)
{
	Intertask_Param_Message_t entry;
	entry.message_type = message_log_ascent_parameters;
	entry.time_stamp = GET_TIME_STAMP;
	entry.message.log_ascent_parameters = *log_ascent_parameters;
	queue_add_blocking(&log_params_queue, &entry);
}

void message_log_descent_params(Log_Descent_Parameters_t *log_descent_parameters)
{
	Intertask_Param_Message_t entry;
	entry.message_type = message_log_descent_parameters;
	entry.time_stamp = GET_TIME_STAMP;
	entry.message.log_descent_parameters = *log_descent_parameters;
	queue_add_blocking(&log_params_queue, &entry);
}

bool message_log_get_params(Intertask_Param_Message_t *log_params)
{
	return queue_try_remove(&log_params_queue, log_params);
}

void message_send_log(char *format, ...)
{
	Log_Message_t entry;
	va_list args;
	va_start (args, format);
	vsnprintf(entry.log_message, MAX_LOG_MESSAGE_SIZE, format, args);
	va_end(args);
	
	entry.time_stamp = GET_TIME_STAMP;
	queue_add_blocking(&log_queue, &entry);
}

bool message_get_log(Log_Message_t *log_message)
{
	return queue_try_remove(&log_queue, log_message);
}