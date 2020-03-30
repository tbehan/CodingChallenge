#pragma once
#include <stdint.h>

typedef int semaphore_t;
typedef int mutex_t;
typedef int msgQueue_t;

/**
 * Spawns a task
 *
 * @param priorty Priority of the task (lower is higher priority)
 * @param stackSize Size of the stack to allocate for the task
 */
void rtos_taskSpawn(void (*fn)(void), unsigned priority, size_t stackSize);

/**
 * Creates a counting semaphore with the maximum count value specified, and an initial count of 0
 *
 * @returns A handle for the semaphore
 */
semaphore_t rtos_semCreate(unsigned count);

/**
 * Attempts to take the semaphore. Blocks for up to @param timeoutMsec
 *
 * @return 0 if semaphore taken, -1 otherwise
 */
int rtos_semTake(semaphore_t sem, unsigned timeoutMsec);

/**
 * Gives the semaphore
 *
 * Can be called from ISR
 */
void rtos_semGive(semaphore_t sem);

/**
 * Creates a mutex
 *
 * @return a handle to the mutex
 */
mutex_t rtos_mtxCreate(void);

/**
 * Attempt to lock the mutex, block for up to @param timeoutMsec
 *
 * @return 0 on lock, -1 otherwise
 */
int rtos_mtxTake(mutex_t mtx, unsigned timeoutMsec);

/**
 * Give the mutex
 */
void rtos_mtxGive(mutex_t mtx);

/**
 * Creates a thread-safe message queue with @param count entries each of size @param entrySize
 *
 * @param count      Number of entries in the queue
 * @param entrySize  Size of each entry in the queue
 *
 * @return a handle to the empty queue
 */
msgQueue_t rtos_msgQueueInit(unsigned cnt, size_t entrySize);

/**
 * Push a new entry into the thread-safe message queue
 *
 * Can be called from an ISR
 *
 * @param data Pointer to the data to copy into the queue
 * @param size Size of the data pointed to by @param data
 *
 * @return 0 on success, -1 if queue is full or entry size mismatch
 */
int rtos_msgQueueSend(msgQueue_t queue, const void *data, size_t size);

/**
 * Pops the next entry from the head of the thread-safe message queue
 *
 * Blocks up to @param timeoutMsec waiting for data
 *
 * @param timeoutMsec Amount of time to wait for queue to have entries
 * @param dataOut     Where to copy the next entry into
 * @param dataSize    The size of the buffer pointed to by @param dataOut
 *
 * @return 0 on success, -1 if nothing was copied into dataOut
 */
int rtos_msgQueueReceive(msgQueue_t queue, void *dataOut, size_t dataSize, unsigned timeoutMsec);

/**
 * Put the calling task to sleep for this period of time
 */
void rtos_taskSleep(unsigned durationMsec);

/**
 * Starts the RTOS scheduler.
 *  Priority based pre-emption. Assume round-robin time-splicing for same priority tasks.
 *
 * This routine does not return
 */
void rtos_startScheduler(void);
