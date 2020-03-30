#include "hardware.h"
#include "rtos.h"

/**
 * Polls the elevator motor and tells it where to go
 *
 * Motor Controller:
 *  I2C Bus: 0 Address: 0x1e BusSpeed: 100khz
 *      TX Byte0: Desired Floor Number: Must be in range [0-10]
 *      RX Byte0: Current Floor number in range [0-10]
 *         Byte1: Current Direction:
 *                  0xff - Down
 *                  0x00 - No motion
 *                  0x01 - Up
 */
void controlTask(void)
{

}

/**
 * Poll Panel 1 for any new floor requests @ 100Hz
 *
 * Once a request is read, it is cleared from panel 1's internal queue.
 *
 * I2C Bus: 0 Address: 0x1d BusSpeed: 100Khz
 *      RX Byte0: Next requested Floor number
 *                  0xff: No Requests
 *                  Otherwise a value between [0-10]
 */
void panel1Task(void)
{

}

/**
 * Waits for a GPIO interrupt to indicate a new message and then reads
 * the request from Panel 2
 *
 * The panel only needs to be read from when a GPIO interrupt occurs.
 *
 * GPIO Port A pin 0
 *      Falling Edge indicates a new request.
 *
 * I2C Bus: 0 Address: 0x1c BusSpeed: 100Khz
 *      RX Byte0: Last Requested Floor number [0-10]
 */
void panel2Task(void)
{

}

/**
 * Main entry point,
 * Initialize the system
 */
int main(void)
{

}
