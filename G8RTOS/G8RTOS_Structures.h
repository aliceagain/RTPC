/*
 * G8RTOS_Structure.h
 *
 *  Created on: Jan 12, 2017
 *      Author: Raz Aloni
 */

#ifndef G8RTOS_STRUCTURES_H_
#define G8RTOS_STRUCTURES_H_

#include "G8RTOS.h"
#include "G8RTOS_Semaphores.h"
#include <stdint.h>

/*********************************************** Constants ****************************************************************************/

#define MAX_NAME_LENGTH 16
typedef uint32_t threadId_t;

/*********************************************** Constants ****************************************************************************/

/*********************************************** Data Structure Definitions ***********************************************************/

/*
 *  Thread Control Block:
 *      - Every thread has a Thread Control Block
 *      - The Thread Control Block holds information about the Thread Such as the Stack Pointer, Priority Level, and Blocked Status
 *      - For Lab 2 the TCB will only hold the Stack Pointer, next TCB and the previous TCB (for Round Robin Scheduling)
 */

/* Create tcb struct here */
struct tcb_t{
    int32_t* sp;
    struct tcb_t* next;
    struct tcb_t* prev;
    semaphore_t* blockedSemaphore;
    uint32_t sleepCount;
    int asleep;
    uint8_t priority;
    int isAlive;
    char threadname[MAX_NAME_LENGTH];
    threadId_t threadID;
};

/*
 *  Periodic Thread Control Block:
 *      - Holds a function pointer that points to the periodic thread to be executed
 *      - Has a period in ms
 *      - Holds Current time
 *      - Contains pointer to the next periodic event - linked list
 */

/* Create periodic thread struct here */
struct periodicThread_t{
    void (*handler)(void);
    uint32_t period;
    uint32_t lastExecuted;
    struct periodicThread_t* next;
    struct periodicThread_t* prev;
};

/*********************************************** Data Structure Definitions ***********************************************************/


/*********************************************** Public Variables *********************************************************************/

struct tcb_t * CurrentlyRunningThread;

/*********************************************** Public Variables *********************************************************************/




#endif /* G8RTOS_STRUCTURES_H_ */
