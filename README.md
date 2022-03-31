Model Rocket Data Collector and Controller

This repository is the ongoing build of model rocket data collection system and (hopefully) controller.  Here are the requirements:

1) Record altitude above ground on a one second interval, retrievable after landing.
2) Record Z-axis (vertical) velocity on a one second interval, retrievable after landing.
3) Record Z-axis acceleration on a one second interval, retrievable after landing.
4) Record up to 10 minutes of data starting at liftoff and ending at touchdown on a non-volatile media.


Secondary requirements:
1) Be capable of recording X-axis and Y-axis velocity on a one second interval, retrievable after landing
2) Be capable of recording X-axis and Y-axis acceleration on a one second interval, retrievable after landing
3) Be capable of deploying a recovery system, either as a single deploy or a dual deploy at a configurable altitude.
4) Be capable of steering the rocket assembly back to the launch pad.
5) Be capable of active airframe guidance during the ascent phase.
6) Be capable of transmitting telemetry during the whole flight.
7) Be capable of air starting.

Hopefully, this repository will eventually have reference implementation schematics, BOMs, Gerber files and code.

I have selected the Raspberry Pi Pico as the platform to build on, mainly because I want to learn its capabilities and it looks rather interesting.