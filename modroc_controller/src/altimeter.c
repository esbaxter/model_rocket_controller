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

File:  altimeter.c

Implements an altitude  function using the MEMS temperature, pressure and humidity chips.

So the idea behind this was to use the BME 280 chip, grab a base pressure reading
and then at a given time interval grab an update pressure reading, compare the
two using a rearrangement of the barometric formula to get a change in altitude.

Kalman filters are used to smooth out noise in both the pressure readings and
the calculated altitude
*/

#include "pico/stdlib.h"
#include "altimeter.h"
#include "barometer.h"
#include <math.h>

#define MEMS_BAROMETER_MEASUREMENT_ERROR 		1.0
#define MEMS_BAROMETER_INITIAL_ESTIMATE_ERROR 	1.0
#define MEMS_BAROMETER_INITIAL_KALMAN_GAIN		1.0
#define MEMS_BAROMETER_Q_FACTOR					0.01
#define MEMS_BAROMETER_CONVERGENCE_LOOP_COUNT 	10

#define MAGIC_EXPONENT				0.1902225603956629  //Basically 1/5.22
#define MAGIC_MULTIPLIER			44330

#define ALTITUDE_MEASUREMENT_ERROR		1.0
#define ALTITUDE_INITIAL_ESTIMATE_ERROR	1.0
#define ALTITUDE_INITIAL_KALMAN_GAIN	1.0
#define ALTITUDE_Q_FACTOR				0.01

#define STANDARD_MSL_HPASCALS		101325.0

#define ALTITUDE_WAIT_TIME 		10 //In milliseconds

typedef struct Kalman_Data {
	double measurement_error;
	double estimate_error;	
	double last_estimate;
	double estimate;
	double kalman_gain;
	double q_factor;
} Kalman_Filter_Data;

static double base_pressure[BAROMETER_NUMBER_SUPPORTED_DEVICES];
static Kalman_Filter_Data kalman_filter_data[BAROMETER_NUMBER_SUPPORTED_DEVICES];
static Kalman_Filter_Data current_altitude;

static uint32_t barometer_count = 0;
static uint32_t barometer_ids[BAROMETER_NUMBER_SUPPORTED_DEVICES];

static void reset_kalman_filter_pressure_data(int32_t id)
{
	kalman_filter_data[id].measurement_error = MEMS_BAROMETER_MEASUREMENT_ERROR;
	kalman_filter_data[id].estimate_error = MEMS_BAROMETER_INITIAL_ESTIMATE_ERROR;
	kalman_filter_data[id].last_estimate = STANDARD_MSL_HPASCALS;
	kalman_filter_data[id].estimate = STANDARD_MSL_HPASCALS;
	kalman_filter_data[id].kalman_gain = MEMS_BAROMETER_INITIAL_KALMAN_GAIN;
	kalman_filter_data[id].q_factor = MEMS_BAROMETER_Q_FACTOR;
}

static void reset_altitude_filter_data()
{
	current_altitude.measurement_error = ALTITUDE_MEASUREMENT_ERROR;
	current_altitude.estimate_error = ALTITUDE_INITIAL_ESTIMATE_ERROR;
	current_altitude.last_estimate = 0; //We assume we haven't moved from the point the base pressure is set
	current_altitude.estimate = 0; //We assume we haven't moved from the point the base pressure is set
	current_altitude.kalman_gain = ALTITUDE_INITIAL_KALMAN_GAIN;
	current_altitude.q_factor = ALTITUDE_Q_FACTOR;
}

// update_estimate is based on information available at kalmanfilter.net
static void update_estimate(double measure, Kalman_Filter_Data *filter_data)
{	
	filter_data->kalman_gain = filter_data->estimate_error/(filter_data->estimate_error + filter_data->measurement_error);
	filter_data->estimate = filter_data->last_estimate + (filter_data->kalman_gain * (measure - filter_data->last_estimate));
	filter_data->estimate_error = (1.0 - filter_data->kalman_gain)*filter_data->estimate_error +
				fabs(filter_data->last_estimate - filter_data->estimate) * filter_data->q_factor;
	filter_data->last_estimate = filter_data->estimate;
	return;
}

//Update the Kalman filter for each barometer.
static Error_Returns get_filtered_readings()
{
	Error_Returns to_return = RPi_Success;

	do
	{
		for(uint32_t count = 0; count < barometer_count; count++)
		{
			uint32_t raw_pressure;

			to_return = barometer_get_current_pressure(barometer_ids[count], &raw_pressure);
			raw_pressure /= 10;
			if (to_return != RPi_Success)
			{
				printf("altitude_package: get_filtered_readings failed: %u\n", to_return);
				break;
			}
			//Need to convert raw_pressure from Q24.8 to double
			update_estimate((double)(raw_pressure/10), &kalman_filter_data[barometer_ids[count]]);
		}
	} while(0);
	return to_return;
}

static double convert_pressure_to_altitude(double base_pressure, double current_pressure)
{
	//This equation is the barometric formula from the Bosch BMP180 datasheet.
	//This function returns the difference between the base pressure and the current pressure
	//in meters.
	return MAGIC_MULTIPLIER * (1-pow((current_pressure/base_pressure), MAGIC_EXPONENT));

}

/* This function assumes sole access to the barometers */
static Error_Returns reset_base_pressure()
{
	Error_Returns to_return = RPi_Success;
	do
	{
		for (uint32_t count = 0; count < barometer_count; count++)
		{
			reset_kalman_filter_pressure_data(barometer_ids[count]);
		}

		//Find a stable value for the at rest pressure
		for (unsigned int count = 0; count < MEMS_BAROMETER_CONVERGENCE_LOOP_COUNT; count++)
		{
			sleep_ms(ALTITUDE_WAIT_TIME);
			to_return = get_filtered_readings();			
			if (to_return != RPi_Success)
			{
				printf("altitude_package: reset_base_pressure() failed to get filtered reading: %u\n", to_return);
				break;
			}
		}

		for (uint32_t count = 0; count < barometer_count; count++)
		{
			base_pressure[barometer_ids[count]] = kalman_filter_data[barometer_ids[count]].estimate;
		}
	} while(0);
	return to_return;
}

Error_Returns altimeter_initialize(uint32_t *barometer_id_array, uint32_t number_of_barometers)
{
	Error_Returns to_return = RPi_InvalidParam;
	do
	{
			
		if (number_of_barometers > BAROMETER_NUMBER_SUPPORTED_DEVICES)
		{
			break;
		}
		
		barometer_count = number_of_barometers;	
		for(uint32_t count = 0; count < barometer_count; count++)
		{
			barometer_ids[count] = barometer_id_array[count];
		}
				
		to_return = altimeter_reset();
	}
	while(0);
	
	return to_return;
}

//Resets the base pressure to the current stable pressure readings
Error_Returns altimeter_reset()
{
	Error_Returns to_return = RPi_Success;

	do
	{
		to_return = reset_base_pressure();
		if (to_return != RPi_Success)
		{
			printf("altimeter: reset_base_pressure failed: %u\n", to_return);
			break;
		}

		reset_altitude_filter_data();
	} while(0);

	return to_return;
}

Error_Returns altimeter_update_altitude()
{
	return get_filtered_readings();
}

//Returns the current difference between the base altitude that is obtained at start up
//or after a call to altitude_reset.

double altimeter_get_delta()
{
	for (uint32_t count = 0; count < barometer_count; count++)
	{
		double altitude;

		altitude = convert_pressure_to_altitude(base_pressure[barometer_ids[count]],
				kalman_filter_data[barometer_ids[count]].estimate);
		update_estimate(altitude, &current_altitude);
	}

	return current_altitude.estimate;
}
