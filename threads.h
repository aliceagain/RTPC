/*
 * threads.h
 *
 *  Created on: Feb 27, 2019
 *      Author: Alice
 */

#ifndef THREADS_H_
#define THREADS_H_

#include <BSP.h>
#include <G8RTOS_Semaphores.h>
#include <G8RTOS_Scheduler.h>
#include <G8RTOS_IPC.h>
#include <msp.h>
#include <G8RTOS.h>
#include <stdio.h>
#include "LCDLib.h"

semaphore_t i2c;
semaphore_t lcd;

#define MAX_BALLS 20
#define BALL_PRIORITY 10
#define TAP_PRIORITY 80
#define ACCEL_PRIORITY 90
#define IDLE_PRIORITY 200
#define TOUCH_FIFO 0

void accelerometer(void);

void lcdTapHandler(void);

void tapResponse(void);

void ButtonHandler(void);

void idleThread(void);

void ballThread(void);

struct ball{
    int16_t positionX;
    int16_t positionY;
    int16_t velocityX;
    int16_t velocityY;
    bool isAlive;
    threadId_t id;
    uint16_t color;
    uint16_t scale;
};

#endif /* THREADS_H_ */
