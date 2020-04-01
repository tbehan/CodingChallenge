#include "hardware.h"
#include "rtos.h"


/*-----------------------------------------------------------------------------
 *  Defines
 *-----------------------------------------------------------------------------*/
#define ELV_NUM_FLOORS 11
#define ELV_TOP_FLOOR 10
#define ELV_PASSENGER_TIME_MS 10000
#define PANEL_UPDATE_INTERVAL_MS   50
#define CONTROL_UPDATE_INTERVAL_MS 100
#define MUTEX_TIMEOUT_DEFAULT 500
#define RTOS_DEFAULT_STACK_SIZE 100

#define I2C_BUS_SPEED_HZ 100000
#define I2C_CONTROL_ADDR 0x1e
#define I2C_PANEL1_ADDR  0x1d
#define I2C_PANEL2_ADDR  0x1c

/*-----------------------------------------------------------------------------
 *  Global Variables
 *-----------------------------------------------------------------------------*/
mutex_t i2c_mtx;

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
 *  Elevator Helper Functions
 *
 *  Note: Elevator helper functions do not handle thread safety. 
 *        They must be called in a thread safe manner.
 *-----------------------------------------------------------------------------*/

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  elv_get_direction
 *  Description:  Determine the direction the elevator should go based on the
 *                current request status of all the floors. If no requests
 *                default to ground floor (floor 0).
 *
 *       Inputs:  None.
 *
 *      Returns:  elevator_dir_t new elevator direction.
 * =====================================================================================
 */
static elv_direction_t elv_get_direction(void)
{
    int i;
    int count;

    if ((ELV_DIR_UP == elv_status.direction) || (ELV_DIR_STOP == elv_status.direction))
    {
        /* If going up or stopped, check for requests above */
        count = 0;
        for (i = ELV_NUM_FLOORS; i > elv_status.current_floor; i--)
        {
            if (FLOOR_STOP == elv_status.floor_request[i])
                count++;
        }

        /* If there are requests above go up */
        if (0 != count)
            return ELV_DIR_UP;
    }

    if ((ELV_DIR_DOWN == elv_status.direction) || (ELV_DIR_STOP == elv_status.direction))
    {
        /* If going down or stopped, check requests below */
        count = 0;
        for (i = 0; i < elv_status.current_floor; i++)
        {
            if (FLOOR_STOP == elv_status.floor_request[i])
                count++;
        }

        /* If there are requests below go down */
        if (0 != count)
            return ELV_DIR_DOWN;
    }

    /* If there are no requests default to ground floor */

    /* If already at ground floor stop */
    if (0 == elv_status.current_floor)
        return ELV_DIR_STOP;

    /* If going to ground floor but not at ground floor 
     * only possible direction is down */
    return ELV_DIR_DOWN;

}		/* -----  end of function elv_get_direction  ----- */

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
    elv_status.floor_request[request] = FLOOR_STOP;
    elv_status.direction = elv_get_direction();

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
 *      Returns: State of the request that was just cleared
 * =====================================================================================
 */
static floor_request_t elv_clear_request(unsigned int request)
{
    /* Do nothing if request is not a valid floor.*/
    if (ELV_NUM_FLOORS < request)
        return FLOOR_SKIP;

    /* Get the current state of the request that is being cleared */
    floor_request_t request_cleared = elv_status.floor_request[request];

    elv_status.floor_request[request] = FLOOR_SKIP;
    elv_status.direction = elv_get_direction();

    return request_cleared;
}		/* -----  end of function elv_clear_request  ----- */


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
    int status;
    uint8_t rx_array[2];
    floor_request_t this_floor_request;
    uint8_t this_floor = 0;
    uint8_t last_floor = 0;
    uint8_t next_floor;

    while (1)
    {
        /* Determine the next floor to move to */
        rtos_mtxTake(elv_status.mtx, MUTEX_TIMEOUT_DEFAULT);
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

        /* Start I2C Critical Section */
        rtos_mtxTake(i2c_mtx, MUTEX_TIMEOUT_DEFAULT);
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
        /* End I2C Mutex Section */

        this_floor = rx_array[0];
        this_floor_request = FLOOR_SKIP;

        /* Update elevator status if current floor has changed */
        if (last_floor != this_floor)
        {
            last_floor = this_floor;

            rtos_mtxTake(elv_status.mtx, MUTEX_TIMEOUT_DEFAULT);
            this_floor_request = elv_clear_request(this_floor);
            rtos_mtxGive(elv_status.mtx);

        }

        /* If this floor requested a stop, stop for passengers */
        if (FLOOR_STOP == this_floor_request)
        {
            /* Request the current floor to stop the elevator */

            /* Start I2C Critical Section */
            rtos_mtxTake(i2c_mtx, MUTEX_TIMEOUT_DEFAULT);
            status = hw_i2cWriteRead(
                    I2C_CONTROL_ADDR,
                    &this_floor,
                    sizeof(this_floor),
                    rx_array,
                    sizeof(rx_array)
                    );
            if (0 != status)
            {
                /* Handle i2c error here */
            }
            rtos_mtxGive(i2c_mtx);
            /* End I2C Mutex Section */

            /* Stop for a set amount of time for passengers */
            rtos_taskSleep(ELV_PASSENGER_TIME_MS);
        }
        else
        {
            /* Don't spam the i2c constantly. 
             * Sleep so other tasks can use the i2c. */
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
    int status;
    uint8_t rx_data;
    uint8_t tx_data;

    while (1)
    {
        /* Start I2C Critical Section */
        rtos_mtxTake(i2c_mtx, MUTEX_TIMEOUT_DEFAULT);

        tx_data = 0;
        status = hw_i2cWriteRead(
                I2C_PANEL1_ADDR,
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

        /* if request is valid, update elevator status */
        if (0xff != rx_data)
        {
            rtos_mtxTake(elv_status.mtx, MUTEX_TIMEOUT_DEFAULT);
            elv_add_request(rx_data);
            rtos_mtxGive(elv_status.mtx);
        }

        /* Don't spam the i2c constantly. 
         * Sleep so other tasks can use the i2c. */
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

}

/**
 * Main entry point,
 * Initialize the system
 */
int main(void)
{
    /* Initialize Hardware */
    if (0 != hw_i2cInit(I2C_BUS_0, I2C_BUS_SPEED_HZ))
    {
        /* Handle I2C bus error here */
    }

    /* Initailize global variables */
    elv_status.mtx = rtos_mtxCreate();
    elv_status.current_floor = 0;
    elv_status.direction = ELV_DIR_STOP;
    for (int i = 0; i < ELV_NUM_FLOORS; i++)
        elv_status.floor_request[i] = FLOOR_SKIP;

    i2c_mtx = rtos_mtxCreate();

    /* panel1Task and controlTask have the same priority and so should alternate
     * between each other as they grab control of the i2c mutex */
    rtos_taskSpawn(panel1Task, 1, RTOS_DEFAULT_STACK_SIZE);
    rtos_taskSpawn(controlTask, 1, RTOS_DEFAULT_STACK_SIZE);
    rtos_startScheduler();

    /* Idle */
    while(1){};



    return 0;
}
