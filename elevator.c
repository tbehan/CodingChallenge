#include "hardware.h"
#include "rtos.h"

/*-----------------------------------------------------------------------------
 *  Defines
 *-----------------------------------------------------------------------------*/
#define ELV_NUM_FLOORS 11
#define ELV_TOP_FLOOR (ELV_NUM_FLOORS - 1)
#define ELV_PASSENGER_TIME_MS 10000
#define PANEL_UPDATE_INTERVAL_MS   50
#define CONTROL_UPDATE_INTERVAL_MS 50
#define MUTEX_TIMEOUT_DEFAULT_MS 500
#define SEM_TIMEOUT_DEFAULT_MS 500
#define RTOS_DEFAULT_STACK_SIZE 100

#define I2C_BUS_SPEED_HZ 100000
#define I2C_CONTROL_ADDR 0x1e
#define I2C_PANEL1_ADDR  0x1d
#define I2C_PANEL2_ADDR  0x1c

/*-----------------------------------------------------------------------------
 *  Global Variables
 *-----------------------------------------------------------------------------*/
mutex_t i2c_mtx;
semaphore_t gpio_a_sem;

typedef enum
{
    FLOOR_SKIP = 0,
    FLOOR_STOP = 1,
} floor_request_t;

typedef enum
{
    ELV_DIR_STOP = 0,
    ELV_DIR_UP = 1,
    ELV_DIR_DOWN = 2,
} elv_direction_t;

struct
{
    mutex_t mtx;
    unsigned int current_floor;
    elv_direction_t direction;
    floor_request_t floor_request[ELV_NUM_FLOORS];
} elv_status;


/*-----------------------------------------------------------------------------
 *  Interrupt Service Routines (ISR)
 *-----------------------------------------------------------------------------*/

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  isr_gpio_a
 *  Description:  Handle interrupts on GPIO port A.
 *
 *         Note:  Assuming interrupt is called per port and not per pin.
 *
 *       Inputs:  None.
 *      Returns:  None.
 * =====================================================================================
 */
void isr_gpio_a(void)
{
    /* Handle only the interrupt on Pin 0 */
    if (true == hw_gpioIrqStatus(GPIO_PORT_A, GPIO_PIN_0))
    {
        /* Clear interrupt */
        hw_gpioIrqClear(GPIO_PORT_A, GPIO_PIN_0);

        rtos_semGive(gpio_a_sem);
    }

    return;
}		/* -----  end of function isr_gpio_a  ----- */

/*-----------------------------------------------------------------------------
 *  Elevator Helper Functions
 *-----------------------------------------------------------------------------*/

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  elv_update_direction
 *  Description:  Determine the direction the elevator should go based on the
 *                current request status of all the floors. If no requests
 *                default to ground floor (floor 0).
 *
 *       Inputs:  None.
 *
 *      Returns:  None.
 * =====================================================================================
 */
static void elv_update_direction(void)
{
    int i;
    int count;

    rtos_mtxTake(elv_status.mtx, MUTEX_TIMEOUT_DEFAULT_MS);

    /* If going up or stopped, check for requests above */
    if ((ELV_DIR_UP == elv_status.direction) || (ELV_DIR_STOP == elv_status.direction))
    {
        count = 0;
        for (i = ELV_NUM_FLOORS; i > elv_status.current_floor; i--)
        {
            if (FLOOR_STOP == elv_status.floor_request[i])
                count++;
        }

        /* If there are requests above go up */
        if (0 != count)
            elv_status.direction = ELV_DIR_UP;
    }

    /* If going down or stopped, check requests below */
    else if ((ELV_DIR_DOWN == elv_status.direction) || (ELV_DIR_STOP == elv_status.direction))
    {
        count = 0;
        for (i = 0; i < elv_status.current_floor; i++)
        {
            if (FLOOR_STOP == elv_status.floor_request[i])
                count++;
        }

        /* If there are requests below go down */
        if (0 != count)
            elv_status.direction = ELV_DIR_DOWN;
    }

    /* If there are no requests default to ground floor */
    else
    {
        /* If already at ground floor stop */
        if (0 == elv_status.current_floor)
            elv_status.direction = ELV_DIR_STOP;

        /* If going to ground floor but not at ground floor 
         * only possible direction is down */
        else
            elv_status.direction = ELV_DIR_DOWN;
    }

    rtos_mtxGive(elv_status.mtx);

    return;
}		/* -----  end of function elv_update_direction  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  elv_add_request
 *  Description:  Handle an incoming request to stop at a floor.
 *
 *       Inputs:  0 -> 10   = Floor to stop at,
 *                Otherwise = Do Nothing.
 *
 *      Returns:  None.
 * =====================================================================================
 */
static void elv_add_request(unsigned int request)
{
    /* Do nothing if request is not a valid floor. */
    if (ELV_NUM_FLOORS < request)
        return;

    /* Update elevator status */
    rtos_mtxTake(elv_status.mtx, MUTEX_TIMEOUT_DEFAULT_MS);
    elv_status.floor_request[request] = FLOOR_STOP;
    rtos_mtxGive(elv_status.mtx);

    /* Update elevator direction */
    elv_update_direction();

    return;
}		/* -----  end of function elv_add_request  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  elv_clear_request
 *  Description:  Handle clearing of floor stop request if one exists for this floor.
 *
 *       Inputs: 0 -> 10   = Request to clear,
 *               Otherwise = Do Nothing.
 *
 *      Returns: State of the floor request that was just cleared
 * =====================================================================================
 */
static floor_request_t elv_clear_request(unsigned int request)
{
    /* Do nothing if request is not a valid floor.*/
    if (ELV_NUM_FLOORS < request)
        return FLOOR_SKIP;

    floor_request_t request_cleared;

    rtos_mtxTake(elv_status.mtx, MUTEX_TIMEOUT_DEFAULT_MS);
    request_cleared = elv_status.floor_request[request];
    elv_status.floor_request[request] = FLOOR_SKIP;
    rtos_mtxGive(elv_status.mtx);

    elv_update_direction();

    return request_cleared;
}		/* -----  end of function elv_clear_request  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  i2c_panel_read
 *  Description:  Read from and i2c panel.
 *
 *       Inputs:  Panel address,
 *      Returns:  Panel data.
 * =====================================================================================
 */
uint8_t i2c_panel_read(uint8_t i2c_addr)
{
    int status;
    uint8_t rx_data;
    uint8_t tx_data = 0;

    /* Start I2C Critical Section */
    rtos_mtxTake(i2c_mtx, MUTEX_TIMEOUT_DEFAULT_MS);
    status = hw_i2cWriteRead(
            i2c_addr,
            &tx_data,
            sizeof(tx_data),
            &rx_data,
            sizeof(rx_data)
            );
    if (0 != status)
    {
        /* Handle i2c error here */
    }
    rtos_mtxGive(i2c_mtx);
    /* End I2C Mutex Section */

    return rx_data;
}		/* -----  end of function i2c_panel_read  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  i2c_control_write
 *  Description:  Write to the elevator motor control the floor to move to.
 *
 *      Inputs:  The floor to move to.
 *     Returns:  The current floor.
 * =====================================================================================
 */
uint8_t i2c_control_write(uint8_t next_floor)
{
    int status;
    uint8_t rx_array[2];

    rtos_mtxTake(i2c_mtx, MUTEX_TIMEOUT_DEFAULT_MS);
    status = hw_i2cWriteRead(
            I2C_CONTROL_ADDR,
            &next_floor,
            sizeof(next_floor),
            rx_array,
            sizeof(rx_array)
            );
    if (0 != status)
    {
        /* Handle i2c error here */
    }
    rtos_mtxGive(i2c_mtx);

    return rx_array[0];
}		/* -----  end of function i2c_control_write  ----- */


/*-----------------------------------------------------------------------------
 *  RTOS Task Functions
 *-----------------------------------------------------------------------------*/

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
    floor_request_t this_floor_request;
    uint8_t this_floor = 0;
    uint8_t last_floor = 0;
    uint8_t next_floor;

    while (1)
    {
        /* Determine the next floor to move to */
        rtos_mtxTake(elv_status.mtx, MUTEX_TIMEOUT_DEFAULT_MS);
        /* Going up and not the top floor */
        if ((ELV_DIR_UP == elv_status.direction) && (ELV_TOP_FLOOR > this_floor))
        {
            next_floor = this_floor + 1;
        }
        /* Going down and not the bottom floor */
        else if ((ELV_DIR_DOWN == elv_status.direction) && (0 < this_floor))
        {
            next_floor = this_floor - 1;
        }
        /* Stopped or at an upper or lower limit */
        else
        {
            next_floor = this_floor;
            elv_status.direction = ELV_DIR_STOP;
        }
        rtos_mtxGive(elv_status.mtx);

        /* Start moving to the next floor */
        this_floor = i2c_control_write(next_floor);


        /* Update elevator status if current floor has changed */
        this_floor_request = FLOOR_SKIP;
        if (last_floor != this_floor)
        {
            last_floor = this_floor;
            this_floor_request = elv_clear_request(this_floor);
        }

        /* If this floor had requested a stop, stop for passengers */
        if (FLOOR_STOP == this_floor_request)
        {
            /* Request the current floor to stop the elevator */
            i2c_control_write(this_floor);

            /* Stop for a set amount of time for passengers */
            rtos_taskSleep(ELV_PASSENGER_TIME_MS);
        }
        else
        {
            /* Don't spam the i2c constantly. 
             * Sleep so other tasks can use the i2c. 
             * This is optional, comment out if not wanted. */
            rtos_taskSleep(PANEL_UPDATE_INTERVAL_MS);
        }

    }
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
    uint8_t panel_data;

    while (1)
    {
        panel_data = i2c_panel_read(I2C_PANEL1_ADDR);

        /* if request is valid, update elevator status */
        if (0xff != panel_data)
            elv_add_request(panel_data);

        /* Don't spam the i2c constantly. 
         * Sleep so other tasks can use the i2c.
         * This is optional, comment out if not wanted. */
        rtos_taskSleep(PANEL_UPDATE_INTERVAL_MS);
    }
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
    int result;
    uint8_t panel_data;

    /* The sem starts taken so this task will block until an interrupt */
    rtos_semTake(gpio_a_sem, SEM_TIMEOUT_DEFAULT_MS);

    while (1)
    {
        /* Block until interrupt releases the semaphore */
        result = rtos_semTake(gpio_a_sem, SEM_TIMEOUT_DEFAULT_MS);

        /* If semaphore was taken read the data from panel2 on the i2c */
        if (0 == result)
        {
            panel_data = i2c_panel_read(I2C_PANEL2_ADDR);

            /* if request is valid, update elevator status */
            if (0xff != panel_data)
                elv_add_request(panel_data);
        }
    }

}

/**
 * Main entry point,
 * Initialize the system
 */
int main(void)
{
    /* Initailize global variables */
    elv_status.mtx = rtos_mtxCreate();
    elv_status.current_floor = 0;
    elv_status.direction = ELV_DIR_STOP;
    for (int i = 0; i < ELV_NUM_FLOORS; i++)
        elv_status.floor_request[i] = FLOOR_SKIP;

    i2c_mtx = rtos_mtxCreate();
    gpio_a_sem = rtos_semCreate(1);

    /* Initialize Hardware */
    if (0 != hw_i2cInit(I2C_BUS_0, I2C_BUS_SPEED_HZ))
    {
        /* Handle I2C bus error here */
    }

    if (0 != hw_gpioInit(GPIO_PORT_A, GPIO_PIN_0))
    {
        /* Handle GPIO error here */
    }
    
    /* Initialize Interrupts */
    hw_gpioIrqInstall(GPIO_PORT_A, GPIO_PIN_0, GPIO_IRQ_TYPE_FALLING_EDGE, isr_gpio_a);
    hw_gpioIrqEnable(GPIO_PORT_A, GPIO_PIN_0, 1);

    /* panel1Task and controlTask have the same priority and so should alternate
     * between each other as they grab control of the i2c mutex */
    rtos_taskSpawn(panel1Task, 2, RTOS_DEFAULT_STACK_SIZE);
    rtos_taskSpawn(controlTask, 2, RTOS_DEFAULT_STACK_SIZE);

    /* panel2Task has a higher priority as it blocks on the GPIO interrupt.
     * And so will only try to use the I2C when it actually needs it. */
    rtos_taskSpawn(panel2Task, 1, RTOS_DEFAULT_STACK_SIZE);

    rtos_startScheduler();

    /* Idle */
    while(1){};

    return 0;
}
