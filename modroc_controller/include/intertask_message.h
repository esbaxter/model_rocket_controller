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

File:  inter_task_message.h

Definitions of messages to passed between the input task and the output task.

*/

#pragma once
#include "stdarg.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "pico/time.h"

#define MAX_LOG_MESSAGE_SIZE 64
#define MAX_NUMBER_MESSAGES 5

#define GET_TIME_STAMP to_ms_since_boot(get_absolute_time())

typedef enum {
	message_log_parameters,
	message_log_message
} Message_Types;

typedef struct Log_Parameters_S
{
	uint32_t time_stamp;
	double altitude;  //In meters
	uint16_t z_acceleration;  // In meters/second2
	uint16_t z_velocity;  //In meters/second
} Log_Parameters_t;

typedef struct Log_Message_S
{
	uint32_t time_stamp;
	char log_message[MAX_LOG_MESSAGE_SIZE];
} Log_Message_t;

typedef union {
	Log_Parameters_t log_parameters;
	Log_Message_t log_message;
} Messages_t;

typedef struct Intertask_Message_S
{
	Message_Types message_type;
	Messages_t message;
	
} Intertask_Message_t;

extern queue_t output_task_queue;

void intertask_message_init();

void intertask_message_send_log(char *format, ...);