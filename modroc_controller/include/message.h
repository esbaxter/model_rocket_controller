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
	message_log_ascent_parameters,
	message_log_descent_parameters
} Param_Message_Types;

typedef struct Log_Ascent_Parameters_S
{
	int32_t altitude;  //In meters
	uint16_t z_acceleration;  // In meters/second2
	uint16_t z_velocity;  //In meters/second
} Log_Ascent_Parameters_t;

typedef struct Log_Descent_Parameters_S
{
	int32_t altitude;  //In meters
	double temperature;  // In degrees C
} Log_Descent_Parameters_t;

typedef struct Log_Message_S
{
	uint32_t time_stamp;
	char log_message[MAX_LOG_MESSAGE_SIZE];
} Log_Message_t;

typedef union {
	Log_Ascent_Parameters_t log_ascent_parameters;
	Log_Descent_Parameters_t log_descent_parameters;
} Param_Messages_t;

typedef struct Intertask_Message_S
{
	Param_Message_Types message_type;
	uint32_t time_stamp;
	Param_Messages_t message;
} Intertask_Param_Message_t;

void message_init();

void message_log_ascent_params(Log_Ascent_Parameters_t *log_ascent_parameters);

void message_log_descent_params(Log_Descent_Parameters_t *log_descent_parameters);

bool message_log_get_params(Intertask_Param_Message_t *log_params);

void message_send_log(char *format, ...);

bool message_get_log(Log_Message_t *log_message);
