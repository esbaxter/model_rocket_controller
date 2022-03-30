Model Rocket Data Collector and Controller

This repository is the ongoing build of model rocket data collection system and (hopefully) controller.  Here are the requirements:

1) Record altitude above ground on a one second interval, retrievable after landing.
2) Record Z-axis (vertical) velocity on a 10 millisecond interval, retrievable after landing.
3) Record Z-axis acceleration on a 10 millisecond interval, retrievable after landing.


Secondary requirements:
1) Be capable of recording X-axis and Y-axis velocity on a 10 millisecond interval, retrievable after landing
2) Be capable of recording X-axis and Y-axis acceleration on a 10 millisecond interval, retrievable after landing
3) Be capable of deploying a recovery system, either as a single deploy or a dual deploy at a configurable altitude.
4) Be capable of steering the rocket assembly back to the lauch pad and land within 10 meters of the original departure point with a horizontal wind component of 4.5 meters/second or less and negligable vertical wind component.
5) Be capable of active airframe guidance during the ascent phase.
6) Be capable of transmitting telemetry during the whole flight.

Obviously some of the requirements are going to depend on the design of the airframe, however, this project will not get too deeply into the airframe.

Hopefully, this repository will eventually have schematics, BOMs, Gerber files and code.

I have selected the Raspberry Pi Pico as the platform to build on, mainly because I want to learn its capabilities and it looks rather interesting.