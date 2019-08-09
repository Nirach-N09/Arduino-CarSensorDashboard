# My fork of Speeddragon's work

Intended for a specific car, my Focus RS. As such there's some customisation around that - The main BMP, and line 144 (currently) has reference to a build number. That can be safely removed or modified with no ill effects.

This only works on a Mega as far as I can make it - It's too large for an Uno as it stands.

I'm not sure if it's normal, but for my implementation the VdoPressureSender.cpp, VdoPressureSender.h and VdoTemperatureSender equivalents sit in the same root directory as the .ino file.

FGTTest.h sits in the Adafruit_GFX_Library/Fonts folder, although could reasonably sit in the same root - I just wanted the font more available than just this project. Can also be substituted for literally any other font as far as I'm aware.

# Arduino - Car Sensor Dashboard

This repository contains C/C++ code to read and display (via Bluetooth) car sensor values. The code was create to get information from:

- Pressure Sender
- Temperature Sender
- MPX4250AP (Pressure sensor)

## Diagram

The layout of the components can be seen here: https://circuits.io/circuits/2527463-car-sensor-controller/

A more detail diagram with all connections, including bluetooth and MPX4250AP will be added latter.

## Sensors

I have bought 3 eBay senders, two temperature sender and one pressure sender.

* 1/8 NPT Oil/Water TEMPERATURE TEMP Sender Sensor fits Autometer VDO Smiths Gauge (**hosefittingsuk**)
* Oil Pressure Electronic Sensor/Sender Replacement Universal Fit Gauge 1/8" NPT (**mcgillmotorsports**)
* Aftermarket Gauge Universal 1/8 NPT Oil / Water Temp Temperature Sensor Sender (**aggressive_gauges_ltd**)

All the calculations with voltage divider and sensor mapping is done in this google drive sheet: https://docs.google.com/spreadsheets/d/1dIEgK2CFQV0UKZFhlpiRPlZvceNms4iL4nMmq7J9Fz0

## Installation

* Copy the folders inside folder **lib** to your arduino libraries.
* Open **car_sensor_dashboard.ino** inside car_sensor_dashboard folder.
