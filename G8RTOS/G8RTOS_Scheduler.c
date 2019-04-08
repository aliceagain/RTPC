/*
 * G8RTOS_Scheduler.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include "G8RTOS_Scheduler.h"
#include "G8RTOS_CriticalSection.h"
#include "driverlib.h"
#include <stdint.h>
#include <string.h>
#include "BSP.h"

/*
 * G8RTOS_Start exists in asm
 */
extern void G8RTOS_Start();

/* System Core Clock From system_msp432p401r.c */
extern uint32_t SystemCoreClock;

/*
 * Pointer to the currently running Thread Control Block
 */
extern struct tcb_t * CurrentlyRunningThread;

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Defines ******************************************************************************/

/* Status Register with the Thumb-bit Set */
#define THUMBBIT 0x01000000

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/* Thread Control Blocks
 *	- An array of thread control blocks to hold pertinent information for each thread
 */
static struct tcb_t threadControlBlocks[MAX_THREADS];

/* Thread Stacks
 *	- An array of arrays that will act as invdividual stacks for each thread
 */
static int32_t threadStacks[MAX_THREADS][STACKSIZE];

/* Periodic Thread Control Blocks
 *  - An array of periodic thread control blocks to hold pertinent information for each thread
 */
static struct periodicThread_t periodicThreadControlBlocks[MAX_PERIODIC_THREADS];

/*********************************************** Data Structures Used *****************************************************************/


/*********************************************** Private Variables ********************************************************************/

/*
 * Current Number of Threads currently in the scheduler
 */
static uint32_t NumberOfThreads;
/*
 * Current Number of Periodic Threads currently in the scheduler
 */
static uint32_t NumberOfPeriodicThreads;

static uint16_t IDCounter;

/*********************************************** Private Variables ********************************************************************/


/*********************************************** Private Functions ********************************************************************/

/*
 * Initializes the Systick and Systick Interrupt
 * The Systick interrupt will be responsible for starting a context switch between threads
 * Param "numCycles": Number of cycles for each systick interrupt
 */
static void InitSysTick(uint32_t numCycles){
    SysTick_Config(numCycles);
}

/*
 * Chooses the next thread to run..
 */
void G8RTOS_Scheduler()
{
    uint8_t currentMaxPriority = 255;
    struct tcb_t * tempNextThread = CurrentlyRunningThread->next;
    while((tempNextThread->blockedSemaphore != 0) || (tempNextThread->asleep == 1)){
        tempNextThread = tempNextThread->next;
    }
    currentMaxPriority = tempNextThread->priority;
    CurrentlyRunningThread = tempNextThread;

    for(int i = 0; i < NumberOfThreads; i++){
        tempNextThread = tempNextThread->next;
        if(tempNextThread->asleep == false && (tempNextThread->blockedSemaphore == 0)){
            if(tempNextThread->priority < currentMaxPriority){
                currentMaxPriority = tempNextThread->priority;
                CurrentlyRunningThread = tempNextThread;
            }
        }
    }
}

/*
 * SysTick Handler
 * Currently the Systick Handler will only increment the system time
 * and set the PendSV flag to start the scheduler
 *
 * In the future, this function will also be responsible for sleeping threads and periodic threads
 */
void SysTick_Handler()
{
    SystemTime++;
    for(int i = 0; i < NumberOfPeriodicThreads; i++){
        if(SystemTime >= (periodicThreadControlBlocks[i].lastExecuted + periodicThreadControlBlocks[i].period)){
            periodicThreadControlBlocks[i].handler();
            periodicThreadControlBlocks[i].lastExecuted = SystemTime + periodicThreadControlBlocks[i].period;
        }
    }
    struct tcb_t * tempNextThread = CurrentlyRunningThread->next;
    for(int i = 0; i < NumberOfThreads; i++){
        tempNextThread = tempNextThread->next;
        if(tempNextThread->asleep && (tempNextThread->sleepCount == SystemTime)){
            tempNextThread->asleep = 0;
        }
    }
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/*
 * Yields Thread by setting PendSV flag
 */
void G8RTOS_Yield(){
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/*********************************************** Private Functions ********************************************************************/


/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Sets variables to an initial state (system time and number of threads)
 * Enables board for highest speed clock and disables watchdog
 */
void G8RTOS_Init()
{
	SystemTime = 0;
	NumberOfThreads = 0;
	NumberOfPeriodicThreads = 0;
	IDCounter = 0;
    CurrentlyRunningThread = &threadControlBlocks[0];
    // Relocate vector table to SRAM to use aperiodic events
    uint32_t newVTORTable = 0x20000000;
    memcpy((uint32_t *)newVTORTable, (uint32_t *)SCB->VTOR, 57*4);
    // 57 interrupt vectors to copy
    SCB->VTOR = newVTORTable;
	BSP_InitBoard();

}

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes the Systick
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
sched_ErrCode_t G8RTOS_Launch()
{
    if(CurrentlyRunningThread->sp != 0){
        G8RTOS_Scheduler();
        InitSysTick( ClockSys_GetSysFreq()/1000 );
        __NVIC_SetPriority(SysTick_IRQn, OSINT_PRIORITY);
        __NVIC_SetPriority(PendSV_IRQn, OSINT_PRIORITY);
        G8RTOS_Start();
    }
    return NO_THREADS_SCHEDULED;
}


/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are still available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread to hold a "fake context"
 * 	- Sets stack tcb stack pointer to top of thread stack
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
sched_ErrCode_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char* threadName)
{
    int32_t IBit_State = StartCriticalSection();
    uint32_t tcbToInitialize = MAX_THREADS;
    for(int i = 0; i < MAX_THREADS; i++){
        if(threadControlBlocks[i].isAlive == false){
            tcbToInitialize = i;
            break;
        }
    }
    if(tcbToInitialize < MAX_THREADS){
        threadControlBlocks[tcbToInitialize].sp = &threadStacks[tcbToInitialize][STACKSIZE-16];

        threadControlBlocks[tcbToInitialize].asleep = 0;
        threadControlBlocks[tcbToInitialize].priority = priority;
        threadControlBlocks[tcbToInitialize].threadID = ((IDCounter++) << 16) | tcbToInitialize;

        threadStacks[tcbToInitialize][STACKSIZE-1] = THUMBBIT;
        threadStacks[tcbToInitialize][STACKSIZE-2] = (uint32_t) threadToAdd;

        if(tcbToInitialize == 0){

            threadControlBlocks[tcbToInitialize].next = &threadControlBlocks[tcbToInitialize];
            threadControlBlocks[tcbToInitialize].prev = &threadControlBlocks[tcbToInitialize];

        }else{

            threadControlBlocks[tcbToInitialize].prev = CurrentlyRunningThread;
            threadControlBlocks[tcbToInitialize].next = CurrentlyRunningThread->next;

            CurrentlyRunningThread->next = &threadControlBlocks[tcbToInitialize];
            threadControlBlocks[tcbToInitialize].next->prev = &threadControlBlocks[tcbToInitialize];

        }
        strcpy(threadControlBlocks[tcbToInitialize].threadname, threadName);
        threadControlBlocks[tcbToInitialize].isAlive = true;

        NumberOfThreads++;
        EndCriticalSection(IBit_State);
        return NO_ERROR;
    }else{
        EndCriticalSection(IBit_State);
        return THREADS_INCORRECTLY_ALIVE;
    }
}

threadId_t G8RTOS_GetThreadId(){
    return CurrentlyRunningThread->threadID;
}

struct tcb_t* G8RTOS_GetThread(threadId_t id){
    for(int i = 0; i < MAX_THREADS; i++){
        if(threadControlBlocks[i].isAlive && threadControlBlocks[i].threadID == id){
            return &threadControlBlocks[i];
        }
    }return NULL;
}

/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param Pthread To Add: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
sched_ErrCode_t G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period)
{
    if(NumberOfPeriodicThreads < MAX_PERIODIC_THREADS){
        periodicThreadControlBlocks[NumberOfPeriodicThreads].handler = PthreadToAdd;
        periodicThreadControlBlocks[NumberOfPeriodicThreads].lastExecuted = SystemTime + NumberOfPeriodicThreads;
        periodicThreadControlBlocks[NumberOfPeriodicThreads].period = period;
        if(NumberOfPeriodicThreads == 0){
            periodicThreadControlBlocks[NumberOfPeriodicThreads].next = &periodicThreadControlBlocks[NumberOfPeriodicThreads];
            periodicThreadControlBlocks[NumberOfPeriodicThreads].prev = &periodicThreadControlBlocks[NumberOfPeriodicThreads];
        }else{
            periodicThreadControlBlocks[NumberOfPeriodicThreads].next = &periodicThreadControlBlocks[0];
            periodicThreadControlBlocks[NumberOfPeriodicThreads].prev = &periodicThreadControlBlocks[NumberOfPeriodicThreads-1];
            periodicThreadControlBlocks[NumberOfPeriodicThreads].next = &periodicThreadControlBlocks[NumberOfPeriodicThreads];
            periodicThreadControlBlocks[0].prev = &periodicThreadControlBlocks[NumberOfPeriodicThreads];
        }
        NumberOfPeriodicThreads++;
        return NO_ERROR;
    }else{
        return THREAD_LIMIT_REACHED;
    }
}


/*
 * Puts the current thread into a sleep state.
 *  param durationMS: Duration of sleep time in ms
 */
void sleep(uint32_t durationMS)
{
    CurrentlyRunningThread->sleepCount = SystemTime + durationMS;
    CurrentlyRunningThread->asleep = true;
    G8RTOS_Yield();
}


sched_ErrCode_t G8RTOS_KillThread(threadId_t threadId){
    int32_t IBit_State = StartCriticalSection();
    if(NumberOfThreads == 0){
        EndCriticalSection(IBit_State);
        return CANNOT_KILL_LAST_THREAD;
    }
    uint32_t tcbToKill = MAX_THREADS;
    for(int i = 0; i < MAX_THREADS; i++){
        if(threadControlBlocks[i].threadID == threadId){
            tcbToKill = i;
            break;
        }
    }
    if(tcbToKill < MAX_THREADS){
        threadControlBlocks[tcbToKill].isAlive = false;
        threadControlBlocks[tcbToKill].next->prev = threadControlBlocks[tcbToKill].prev;
        threadControlBlocks[tcbToKill].prev->next = threadControlBlocks[tcbToKill].next;
        NumberOfThreads--;
        EndCriticalSection(IBit_State);
        if(threadControlBlocks[tcbToKill].threadID == CurrentlyRunningThread->threadID){
            G8RTOS_Yield();
        }
    }else{
        EndCriticalSection(IBit_State);
        return THREAD_DOES_NOT_EXIST;
    }
    return NO_ERROR;
}

sched_ErrCode_t G8RTOS_KillSelf(){
    return G8RTOS_KillThread(CurrentlyRunningThread->threadID);
}

void G8RTOS_KillOthers(){
    for(int i = 0; i < MAX_THREADS; i++){
        if(threadControlBlocks[i].isAlive && threadControlBlocks[i].threadID != CurrentlyRunningThread->threadID){
            G8RTOS_KillThread(threadControlBlocks[i].threadID);
        }
    }
}

sched_ErrCode_t G8RTOS_AddAPeriodicEvent(void (*AthreadToAdd) (void), uint8_t priority, IRQn_Type IRQn){
    if( IRQn < PSS_IRQn || IRQn > PORT6_IRQn){
        return IRQn_INVALID;
    }
    if( !(priority < OSINT_PRIORITY)){
        return HWI_PRIORITY_INVALID;
    }
    __NVIC_SetVector(IRQn, AthreadToAdd);
    __NVIC_SetPriority(IRQn, priority);
    __NVIC_EnableIRQ(IRQn);

    return NO_ERROR;
}


/*********************************************** Public Functions *********************************************************************/
