# JDRF Coding Challenge

You are tasked with writing a multi-threaded application to read some data from an Ambient Light Sensor (ALS) and display the results using some LEDS.
The application runs on an Atmel Cortex-M4 microcontroller.
The ALS is interfaced to the microcontroller over I2C.

All required datasheets and documentation are provided in the docs folder

Take the next couple minutes to read through the task overview and the template code provided. The comments in the code will describe what needs to be done in more detail. Feel free to ask questions at any point during the challenge.

You are also welcome to use the internet to look up any additional resources or references you may require for this challenge. The following links are probably useful:

- [FreeRTOS API](https://www.freertos.org/a00106.html)

## Part 1:

The application will have 3 tasks/threads. You must implement these threads starting from the template provided.

- Thread 1: Reads a specific register from the ALS device every 500msec and sends the result to the LED thread (Thread #3) for display

- Thread 2: Reads a specific register from the ALS device every 1000msec and sends the result to the LED thread (Thread #3) for display

- Thread 3: Take the results from Threads 1 and 2 and displays them using the available LEDS. Each result is to be displayed for 100msec.

Atmel studio provides a basic I2C driver that can be used for this part of the challenge.
You will need to import the TWI Service into your project.

All other required dependencies are already included in the project.

## Part 2:

The basic I2C driver provided by the atmel ASF is not great. We would like something that is ISR based instead of polling.

Implement an ISR based routine to perform the I2C read operation.

Feel free to start from the ASF provided driver and modify as required.
