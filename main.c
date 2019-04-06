#include "msp.h"

#include <BSP.h>
#include "G8RTOS.h"
#include "LCDLib.h"
#include "Game.h"

/**
 * main.c
 */

void main(void){
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

}
