/*
 * G8RTOS_IPC.c
 *
 *  Created on: Jan 10, 2017
 *      Author: Daniel Gonzalez
 */
#include <stdint.h>
#include "msp.h"
#include "G8RTOS_IPC.h"
#include "G8RTOS_Semaphores.h"

/*********************************************** Defines ******************************************************************************/

#define FIFOSIZE 32
#define MAX_NUMBER_OF_FIFOS 4

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/*
 * FIFO struct will hold
 *  - buffer
 *  - head
 *  - tail
 *  - lost data
 *  - current size
 *  - mutex
 */

struct FIFO_t{
    uint32_t buffer[FIFOSIZE];
    uint32_t* head;
    uint32_t* tail;
    uint32_t lostData;
    semaphore_t currentSize;
    semaphore_t mutex;
};

/* Array of FIFOS */
static struct FIFO_t FIFOs[MAX_NUMBER_OF_FIFOS];

/*********************************************** Data Structures Used *****************************************************************/

/*
 * Initializes FIFO Struct
 */
int G8RTOS_InitFIFO(uint32_t FIFOIndex)
{
    if(FIFOIndex < MAX_NUMBER_OF_FIFOS){
        FIFOs[FIFOIndex].head = FIFOs[FIFOIndex].buffer;
        FIFOs[FIFOIndex].tail = FIFOs[FIFOIndex].buffer;
        FIFOs[FIFOIndex].lostData = 0;
        G8RTOS_InitSemaphore(&FIFOs[FIFOIndex].currentSize,0);
        G8RTOS_InitSemaphore(&FIFOs[FIFOIndex].mutex,1);
        return 1;
    }else{
        return 0;
    }
}

/*
 * Reads FIFO
 *  - Waits until CurrentSize semaphore is greater than zero
 *  - Gets data and increments the head ptr (wraps if necessary)
 * Param: "FIFOChoice": chooses which buffer we want to read from
 * Returns: uint32_t Data from FIFO
 */
uint32_t readFIFO(uint32_t FIFOChoice)
{
    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].currentSize);
    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].mutex);
    uint32_t data = *FIFOs[FIFOChoice].head;
    FIFOs[FIFOChoice].head++;
    if(FIFOs[FIFOChoice].head == &FIFOs[FIFOChoice].head){
        FIFOs[FIFOChoice].head = FIFOs[FIFOChoice].buffer;
    }
    G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].mutex);
    return data;
}

/*
 * Writes to FIFO
 *  Writes data to Tail of the buffer if the buffer is not full
 *  Increments tail (wraps if necessary)
 *  Param "FIFOChoice": chooses which buffer we want to read from
 *        "Data': Data being put into FIFO
 *  Returns: error code for full buffer if unable to write
 */
int writeFIFO(uint32_t FIFOChoice, uint32_t Data)
{
    int returnCode = NO_ERROR;
    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].mutex);
    *FIFOs[FIFOChoice].tail = Data;
    G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].currentSize);
    if(FIFOs[FIFOChoice].currentSize > FIFOSIZE-1){
      FIFOs[FIFOChoice].lostData++;
      FIFOs[FIFOChoice].currentSize = FIFOSIZE-1;
      returnCode = BUFFER_FULL_ERROR;
      FIFOs[FIFOChoice].head++;
      if(FIFOs[FIFOChoice].head == &FIFOs[FIFOChoice].head){
          FIFOs[FIFOChoice].head = FIFOs[FIFOChoice].buffer;
      }
    }
    FIFOs[FIFOChoice].tail++;
    if(FIFOs[FIFOChoice].tail == &FIFOs[FIFOChoice].head){
        FIFOs[FIFOChoice].tail = FIFOs[FIFOChoice].buffer;
    }
    G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].mutex);
    return returnCode;
}

