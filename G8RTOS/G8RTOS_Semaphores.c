/*
 * G8RTOS_Semaphores.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include "G8RTOS_CriticalSection.h"
#include "G8RTOS_Semaphores.h"
#include "G8RTOS_Scheduler.h"
#include "G8RTOS_Structures.h"
#include <stdint.h>
#include "msp.h"

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes a semaphore to a given value
 * Param "s": Pointer to semaphore
 * Param "value": Value to initialize semaphore to
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_InitSemaphore(semaphore_t *s, int32_t value){
    int32_t IBit_State = StartCriticalSection();
    *s = value;
    EndCriticalSection(IBit_State);
}

/*
 * Waits for a semaphore to be available (value greater than 0)
 * 	- Decrements semaphore when available
 * 	- Spinlocks to wait for semaphore
 * Param "s": Pointer to semaphore to wait on
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_WaitSemaphore(semaphore_t *s){
    int32_t IBit_State = StartCriticalSection();
    (*s)--;
    if((*s) < 0){
        CurrentlyRunningThread->blockedSemaphore = s;
        EndCriticalSection(IBit_State);
        G8RTOS_Yield();
    }

    EndCriticalSection(IBit_State);
}

/*
 * Signals the completion of the usage of a semaphore
 * 	- Increments the semaphore value by 1
 * Param "s": Pointer to semaphore to be signaled
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_SignalSemaphore(semaphore_t *s)
{
    int32_t IBit_State = StartCriticalSection();
    (*s)++;

    if((*s) <= 0){
        struct tcb_t *pt = CurrentlyRunningThread->next;
        while(pt->blockedSemaphore !=s){
            pt = pt->next;
        }
        pt->blockedSemaphore = 0;
    }
    EndCriticalSection(IBit_State);
}

/*********************************************** Public Functions *********************************************************************/


