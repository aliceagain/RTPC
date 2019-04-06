#include "msp.h"
#include <BSP.h>
#include "G8RTOS.h"
#include "LCDLib.h"
#include "threads.h"

/**
 * main.c
 */

void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer
	G8RTOS_Init();
	LCD_Init(true);

    G8RTOS_InitSemaphore(&lcd, 1);
    G8RTOS_InitSemaphore(&i2c, 1);
    G8RTOS_InitFIFO(TOUCH_FIFO);
    char* accel = "Accel";
    char* tapHandler = "tapHandler";
    char* idle = "Idle";
    G8RTOS_AddThread(accelerometer, ACCEL_PRIORITY, accel);
    G8RTOS_AddThread(tapResponse, TAP_PRIORITY, tapHandler);
    G8RTOS_AddThread(idleThread, IDLE_PRIORITY, idle);
    G8RTOS_AddAPeriodicEvent(lcdTapHandler, 6, PORT4_IRQn);
//    G8RTOS_AddAPeriodicEvent(ButtonHandler, 5, PORT5_IRQn);
//    P5->DIR &= ~BIT5;
//    P5->IFG &= ~BIT5;//P4.4 IFG cleared
//    P5->IE  |=  BIT5; //Enable interrupt on P4.4
//    P5->IES |=  BIT5; //high-to-low transition
//    P5->REN |=  BIT5; //Pull-up resister
//    P5->OUT |=  BIT5; //Sets res to pull-up

    G8RTOS_Launch();
}
