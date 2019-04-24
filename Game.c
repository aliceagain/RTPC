#include "Game.h"
#include "BSP.h"
#include "stdio.h"
/*********************************************** Global Vars *********************************************************************/
GameState_t state;
char readTouchName[] = "readTouch";
char idlethreadName[] = "idle";
char sendDatatName[] = "sendData";
char receiveDataName[] = "receiveData";
char drawName[] = "Draw";

/*********************************************** Common Threads *********************************************************************/

void Send(){
  while(1){
    G8RTOS_WaitSemaphore(&globalState);
    if(state.lastDrawnStroke != state.lastSentStroke){
      for(; state.lastDrawnStroke != state.lastSentStroke; state.lastSentStroke++){
        SendStroke();
        G8RTOS_SignalSemaphore(&globalState);
        sleep(4);
        G8RTOS_WaitSemaphore(&globalState);
      }
    }else{
      G8RTOS_SignalSemaphore(&globalState);
    }
    sleep(10);
  }
}

void Receive(){
  while(1){
    ReceiveStroke();
    sleep(2);
  }
}

uint16_t touchX;
uint16_t touchY;
void ProcessTouch(){
    G8RTOS_WaitSemaphore(&globalState);
    if(touchX > DRAWABLE_MIN_X && touchX < DRAWABLE_MAX_X &&
       touchY > DRAWABLE_MIN_Y && touchY < DRAWABLE_MIN_Y){
      state.touch.x = touchX - DRAWABLE_MIN_X;
      state.touch.y = touchY - DRAWABLE_MIN_Y;
    }
    G8RTOS_SignalSemaphore(&globalState);
    sleep(4);
}

void ReadTouch(){
  //READ Touch
}

void Draw(){
  uint8_t prevBoard = -1;
  ScreenPos_t lastTouch = (ScreenPos_t){0,0};
  while(1){
    if(prevBoard == state.currentBoard && state.currentBoard == SELF){
      if(lastTouch.x != state.touch.x || lastTouch.y != state.touch.y){
        if((state.lastDrawnStroke+1)%16 == state.lastSentStroke){
          G8RTOS_Yield();
        }else{
          DrawStroke();
        }
      }
    }else if(prevBoard != state.currentBoard){
      DrawBoard();
      RedrawStrokes();
      prevBoard = state.currentBoard;
    }
    sleep(1);
  }
}

/*
 * Idle thread
 */
void IdleThread(){
  while(1){
      G8RTOS_Yield();
  }
}

 bool isHost = false;
 bool isClient = false;

 inline void initInterrupts(){
    //Button Init 4.4 (button 0) -> server/host
    P4->DIR &= ~BIT4;   //set as input
    P4->IFG &= ~BIT4;   //P4.4 IFG cleared
    P4->IE |= BIT4;     //Enable interrupt on P4.4
    P4->IES |= BIT4;    //falling edge triggered
    P4->REN |= BIT4;    //pull-up resistor
    P4->OUT |= BIT4;    //Sets res to pull-up

    //Button Init 5.4 (button 2) -> client
    P5->DIR &= ~BIT4;   //set as input
    P5->IFG &= ~BIT4;   //P5.4 IFG cleared
    P5->IE |= BIT4;     //Enable interrupt on P5.4
    P5->IES |= BIT4;    //falling edge triggered
    P5->REN |= BIT4;    //pull-up resistor
    P5->OUT |= BIT4;    //Sets res to pull-up

    NVIC_EnableIRQ(PORT4_IRQn);
    NVIC_EnableIRQ(PORT5_IRQn);
 }

void PORT4_IRQHandler(){
    isHost = true;  //b0 pressed
    P4->IE &= ~BIT4;
    P5->IE &= ~BIT4;
    P4->IFG &= ~BIT4;  
}

void PORT5_IRQHandler(){
    isClient = true;    //b2 pressed
    P4->IE &= ~BIT4;
    P5->IE &= ~BIT4;
    P5->IFG &= ~BIT4; 
}

void WaitInit(){
    initInterrupts();
    while(!(isHost || isClient));
    G8RTOS_InitSemaphore(&cc3100, 1);
    G8RTOS_InitSemaphore(&lcd, 1);
    G8RTOS_InitSemaphore(&screenSelf, 1);
    G8RTOS_InitSemaphore(&screenFriend, 1);
    G8RTOS_InitSemaphore(&globalState, 1);
    if(isHost){
        InitHost();
    }else if(isClient){
        InitClient();
    }
    CreateThreadsAndStart();
}

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

void InitHost(){
  initCC3100(Host);
  while(ReceiveData((uint8_t * )&state.ip, sizeof(uint32_t)) == NOTHING_RECEIVED);

  uint8_t temp;
  SendData((uint8_t * ) &temp, state.ip, sizeof(uint8_t));

  P2->DIR |= RED_LED;  //p1.0 set to output
  BITBAND_PERI(P2->OUT, 0) = 1;  //toggle led on
}

void InitClient(){
  initCC3100(Client);
  state.ip = getLocalIP();

  SendData((uint8_t *)&state.ip, HOST_IP_ADDR, sizeof(uint32_t));

  uint8_t temp;
  while(ReceiveData(&temp, sizeof(uint8_t)) == NOTHING_RECEIVED);

  P2->DIR |= BLUE_LED;  //p1.0 set to output
  BITBAND_PERI(P2->OUT, 2) = 1;  //toggle led on
}

void CreateThreadsAndStart(){
  while(1);
}

void DrawBoard(){

}

void RedrawStrokes(){

}

void DrawStroke(){

}

void SendStroke(){

}

void ReceiveStroke(){

}

/*********************************************** Public Functions *********************************************************************/
