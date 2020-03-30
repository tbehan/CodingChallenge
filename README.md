# JDRF Coding Challenge

You are tasked with writing the firmware for an elevator controller

The elevator has 3 components to be managed:

    - Motor Controller: An I2C device responsible for controlling the physical position of the elevator car.

    - Panel-1: An i2c device inside the elevator car that users can make floor requests through

    - Panel-2: An i2c device outside the elevator car that provides floor requests made from each floor of the building.

You are to write firmware using at least 3 tasks to be executed on an arbitrary RTOS scheduler.

The firwmare should read the requests from the panels and issue commands to the motor-controller.

We would like the elevator car to satisfy all possible floor requests before changing directions.
For example if the requests come in as 10 2 7 and we are currently at floor 3 we expect the car to stop at floors 7 then 10 before going back down to 2.

You may assume that the elevator car stops at each floor for a minimum duration of 10 seconds.

Once the all floor requests have been satisfied we would like the elevator car to return to the ground floor.

The code should be able to compile to an object file but does not need to run or link.

You may assume a 32bit microcontroller architecture
