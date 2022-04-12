Model Rocket Data Collector and Controller

This repository is the ongoing build of model rocket data collection system and (hopefully) controller.  Here are the requirements:

1) On ascent, log altitude above ground, Z-axis (vertical) velocity and Z-axis acceleration.  Log interval should be configurable between 50 milliseconds and 1000 milliseconds.  Log should be retrievable after landing.
2) At apogee log alititude above ground and temperature.
3) On descent, log altitude above ground and temperature on a one second interval.  Log should be retrievable after landing.
4) Record up to 10 minutes of data starting at liftoff and ending at touchdown on a non-volatile media.


Secondary requirements:
1) On ascent, log X-axis and Y-axis acceleration and X-axis and Y-axis velocity.  Log interval should be configurable between 50 milliseconds and 1000 milliseconds.  Log should be retrievable after landing.
3) Be capable of deploying a recovery system, either as a single deploy or a dual deploy at a configurable altitude.
4) Be capable of steering the rocket assembly back to the launch pad.
5) Be capable of active airframe guidance during the ascent phase.
6) Be capable of transmitting telemetry during the whole flight.
7) Be capable of air starting.

Hopefully, this repository will eventually have reference implementation schematics, BOMs, Gerber files and code.

I have selected the Raspberry Pi Pico as the platform to build on, mainly because I want to learn its capabilities and it looks rather interesting.

Building:

This software package is being developed using the guidance of the Raspberry Pi Pico developers handbook.  To build the software you will need to follow the "Getting Started" section of the guide to obtain the Pico SDK and tinyUSB libraries.  The SDK should be put at the same top level directory as this software.  Currently, I have confirmed it builds correctly on Windows 11.  CMake files are provided which should support building on Linux.

Implementation sequence for primary requirements:

1) Impement basic program structure along with logging.  Complete
2) Implement support for a Bosch BME 280 pressure sensor.  Complete
3) Implement altitude support.  Complete
4) Implement basic state machine for a flight.  Complete
5) Implment thermometer.  Complete
6) Implement support for InvenSense ICM-20948.  In progress
7) Implement kinematics support to determine accelerations and velocities.
8) Implement support for FRAM storage.
9) Implement support for log retrieval.
10) Create schematics for board that supports primary requirements.

Implementation sequence for secondary requirements:
TBD