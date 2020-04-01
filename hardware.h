#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*******************************************************************************
*
* I2C Interface
*
*******************************************************************************/
typedef enum {
    I2C_BUS_0,
} i2cBus_t;

/**
 * Initialize the I2C bus
 *
 * @param frequencyHz  Frequency of the bus in Hz
 *
 * @return 0 on success, -1 otherwise
 */
int hw_i2cInit(i2cBus_t bus, uint32_t frequencyHz);

/**
 * Perform a raw hardware level i2c write/read operation
 *
 * @returns number of bytes read, -1 on error
 */
int hw_i2cWriteRead(i2cBus_t bus, const uint8_t *txData, size_t txCnt, uint8_t *rxDataOut, size_t rxCnt);

/*******************************************************************************
*
* GPIO Interface
*
*******************************************************************************/
typedef enum {
    GPIO_PORT_A,
} gpioPort_t;

typedef enum {
    GPIO_PIN_0,
    GPIO_PIN_1,
} gpioPin_t;

typedef enum {
    GPIO_LOW,
    GPIO_HIGH,
} gpioLevel_t;

typedef enum {
    GPIO_IRQ_TYPE_FALLING_EDGE,
    GPIO_IRQ_TYPE_RISING_EDGE,
    GPIO_IRQ_TYPE_LOW,
    GPIO_IRQ_TYPE_HIGH
} gpioIrqType_t;

/**
 * Initialize a GPIO pin
 *
 * @return 0 on success, -1 otherwise
 */
int hw_gpioInit(gpioPort_t port, gpioPin_t pin);

/**
 * Read the current value of the gpio
 */
gpioLevel_t hw_gpioRead(gpioPort_t port, gpioPin_t pin);

/**
 * Set the level of the gpio
 */
void hw_gpioSet(gpioPort_t port, gpioPin_t pin, gpioLevel_t level);

/**
 * Enable/Disable the gpio interrupt
 */
void hw_gpioIrqEnable(gpioPort_t port, gpioPin_t pin, bool enable);

/**
 * Clear any pending gpio interrupt
 */
void hw_gpioIrqClear(gpioPort_t port, gpioPin_t pin);

/**
 * Returns true if an interrupt is pending
 */
bool hw_gpioIrqStatus(gpioPort_t port, gpioPin_t pin);

/**
 * Install an IRQ handler for the gpio
 *
 * @param type Set the IRQ trigger type
 */
void hw_gpioIrqInstall(gpioPort_t port, gpioPin_t pin, gpioIrqType_t type, void (*irqHandler)(void));

/*******************************************************************************
*
*
* Critical Sections
*
*******************************************************************************/
/**
 * Enter a critical section
 */
void hw_criticalSectionStart(void);
/**
 * Exit a critical section
 */
void hw_criticalSectionEnd(void);

