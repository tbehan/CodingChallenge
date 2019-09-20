#include <stdint.h>
#include <stdbool.h>
#include <asf.h>

#define IO_INDICATOR_LED IOPORT_CREATE_PIN(PIOB, 14)
#define IO_RED_LED		 /* TODO */
#define IO_GREEN_LED	 /* TODO */
#define IO_BLUE_LED		 /* TODO */

#define ALS_DEVICE_ADDRESS  0x10
#define ALS_ALS_REGISTER	/* TODO */
#define ALS_WHITE_REGISTER	/* TODO */

/* Initializes the TWI peripheral for I2C operation */
static void i2cInit(void)
{
    sysclk_enable_peripheral_clock(ID_TWI0);

     /* Pin Configuration */
    ioport_set_pin_mode(IOPORT_CREATE_PIN(PIOA, 3), IOPORT_MODE_MUX_A);
    ioport_set_pin_mode(IOPORT_CREATE_PIN(PIOA, 4), IOPORT_MODE_MUX_A);
    ioport_disable_pin(IOPORT_CREATE_PIN(PIOA, 3));
    ioport_disable_pin(IOPORT_CREATE_PIN(PIOA, 4));

	/* Initialize the TWI0 peripheral as a master here for 100Khz operation. 
	 * The system core clock frequency is made public via SystemCoreClock
	 *	Hint: Start with twi_master_setup() */

}

/* Initializes the LEDs for the board */
static void ledsInit(void)
{		
	static const uint32_t MATRIX_SYSIO_CFG  =  ((1 << 4) | (1 << 5) | (1 << 10) | (1 << 11) | (1 << 12));
	MATRIX->CCFG_SYSIO = MATRIX_SYSIO_CFG;
	
	/* Initialize the indicator LED */
	ioport_set_pin_dir  (IO_INDICATOR_LED, IOPORT_DIR_OUTPUT);
	ioport_set_pin_mode (IO_INDICATOR_LED, IOPORT_MODE_MUX_A);
	ioport_set_pin_level(IO_INDICATOR_LED, false);

	/* Initialize the RGB LEDS here */
}

/*
 * Display data using the RGB LED
 *   Take the 3 least significant bits and map them to the RGB leds 
 */
static void rgbDisplay(unsigned value)
{
	
}

/* This routine reads the ALS register from the Ambient Light Sensor (ALS) I2C Slave Device */
static int alsRead(void)
{
	/* Read the ALS_ALS_REGISTER here:
	 *		Hint: Start with twi_master_read()
	 */
	return -1;
}

/* This routine reads the WHITE register from the Ambient Light Sensor (ALS) I2C Slave Device */
static unsigned whiteRead(void)
{
	/* Read the ALS_WHITE_REGISTER here:
	 *		Hint: Start with twi_master_read()
	 */
	return -1;
}

/*
 * This task should display the data received from alsTask and whiteTask using rgbDisplay()
 * When displaying ALS data the INDICATOR_LED should be ON. 
 * When displaying WHITE data the INDICATOR_LED should be OFF 
 * Each value should be displayed for 100msec.
 */
static void ledTask(void *arg)
{	
	while (1) {
		
	}
}

/*
 * This task should read the ALS_ALS_REGISTER every 1000msec and notify the LED
 * task of the result
 */
static void alsTask(void *arg)
{	
	while (1) {
		vTaskDelay(1000);
		
	}
}

/*
 * This task should read the ALS_WHITE_REGISTER every 500msec and notify the LED
 * task of the result
 */
static void whiteTask(void *arg)
{
	while (1) {
		vTaskDelay(500);
		
	}
}

int main (void)
{
	board_init();
	sysclk_init();

	i2cInit();
	ledsInit();
	
	/* TODO: Launch Tasks and Start the RTOS scheduler */
		
	return 0;
}
