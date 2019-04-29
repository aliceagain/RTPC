#include "Game.h"
#include "BSP.h"
#include "stdio.h"
/*********************************************** Global Vars *********************************************************************/
GameState_t state;
char processTouchName[] = "processTouch";
char idlethreadName[] = "idle";
char sendDataName[] = "sendData";
char receiveDataName[] = "receiveData";
char drawName[] = "Draw";
char joyName[] = "Joystick";

StrokeQueue_t selfQueue;
StrokeQueue_t friendQueue;

/*********************************************** Common Threads *********************************************************************/

void Send(){
  while(1){
    for(; state.stackPos > state.lastSentStroke; state.lastSentStroke++){
      SendStroke(&selfQueue.strokes[state.lastSentStroke]);
      sleep(4);
    }
    sleep(10);
  }
}

void Receive(){
  while(1){
    ReceiveStroke(&selfQueue.strokes[state.friendStackPos]);
    state.friendStackPos++;
    sleep(2);
  }
}

bool touchReceived;
void ProcessTouch(){
  uint16_t touchX;
  uint16_t touchY;
  uint16_t lastX;
  uint16_t lastY;
  while(1){
    if(touchReceived){
      touchX = TP_ReadX();
      touchY = TP_ReadY();
      touchReceived = false;
        G8RTOS_WaitSemaphore(&globalState);
        if(touchX > DRAWABLE_MIN_X && touchX < DRAWABLE_MAX_X &&
           touchY > DRAWABLE_MIN_Y && touchY < DRAWABLE_MAX_Y){
          if(abs((int16_t)touchX-lastX) > MIN_DIST || abs((int16_t)touchX-lastY) > MIN_DIST){
            lastX = touchX;
            lastY = touchY;
            selfQueue.strokes[state.stackPos].pos.x = touchX - DRAWABLE_MIN_X;
            selfQueue.strokes[state.stackPos].pos.y = touchY - DRAWABLE_MIN_Y;
            selfQueue.strokes[state.stackPos].brush.color = state.currentBrush.color;
            selfQueue.strokes[state.stackPos].brush.size = state.currentBrush.size;
            state.stackPos++;
          }
        }
        G8RTOS_SignalSemaphore(&globalState);
    }
    sleep(4);
  }
}

void ReadJoystick(){
    int16_t joyX;
    int16_t joyY;
    while(1){
        GetJoystickCoordinates(&joyX, &joyY);
        if(joyX > 4000 || joyY > 4000 || joyX < -4000 || joyY < -4000){
            state.currentBrush.color.c = (joyX & 0xff00) | (joyY & 0x00ff);
        }
        sleep(10);
    }
}


void Draw(){
  uint8_t prevBoard = -1;
  uint16_t lastStackPosition = 0;
  uint16_t lastFriendStackPosition = 0;
  Brush_t prevBrush;
  while(1){
    if(state.undo){
      for(; state.undo > 0; state.undo--){
        Undo();
      }
    }else{
      if(prevBoard == state.currentBoard && state.currentBoard == SELF){
        if(state.stackPos > lastStackPosition){
          for(; state.stackPos > lastStackPosition; lastStackPosition++){
            DrawStroke(&selfQueue.strokes[lastStackPosition]);
          }
        }else if(state.stackPos < lastStackPosition){
            DrawBoard();
            RedrawStrokes();
        }
      }else if(prevBoard == state.currentBoard && state.currentBoard == FRIEND){
        if(state.stackPos > lastFriendStackPosition){
          for(; state.stackPos > lastFriendStackPosition; lastFriendStackPosition++){
            DrawStroke(&friendQueue.strokes[lastFriendStackPosition]);
          }
        }else if(state.stackPos < lastFriendStackPosition){
            DrawBoard();
            RedrawStrokes();
        }
      }else if(prevBoard != state.currentBoard){
        DrawBoard();
        RedrawStrokes();
        prevBoard = state.currentBoard;
      }
    }
    if(prevBrush.color.c != state.currentBrush.color.c ||
       prevBrush.size != state.currentBrush.size){
      prevBrush.color.c = state.currentBrush.color.c;
       prevBrush.size = state.currentBrush.size;
       DrawInfo();
    }
    sleep(10);
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
    //Button Init 4.4 (b0 - top button) -> server/host AND viewFriendScreen
    //Button Init 4.5 (b1 - right button) -> increasBrush
    P4->DIR &= ~(BIT4 | BIT5);   //set as input
    P4->IFG &= ~(BIT4 | BIT5);   //P4.4 IFG cleared
    P4->IE |= (BIT4 | BIT5);     //Enable interrupt on P4.4
    P4->IES |= (BIT4 | BIT5);    //falling edge triggered
    P4->REN |= (BIT4 | BIT5);    //pull-up resistor
    P4->OUT |= (BIT4 | BIT5);    //Sets res to pull-up

    //Button Init 5.4 (b2 - bottom button) -> client AND viewOwnScreen
    //Button Init 5.5 (b3 - left button) -> decreaseBrush
    P5->DIR &= ~(BIT4 | BIT5);   //set as input
    P5->IFG &= ~(BIT4 | BIT5);   //P5.4 IFG cleared
    P5->IE |= (BIT4 | BIT5);     //Enable interrupt on P5.4
    P5->IES |= (BIT4 | BIT5);    //falling edge triggered
    P5->REN |= (BIT4 | BIT5);    //pull-up resistor
    P5->OUT |= (BIT4 | BIT5);    //Sets res to pull-up

    NVIC_EnableIRQ(PORT4_IRQn);
    NVIC_EnableIRQ(PORT5_IRQn);
 }

bool connected = false;
bool decreaseBrush = false;
bool increaseBrush = false;
void PORT4_IRQHandler(){
    uint16_t cs_value = StartCriticalSection();
    if((P4->IFG & BIT4) && !connected){     //if top button (b0) pressed and not connected yet
        isHost = true;
        connected = true;
        P4->IFG &= ~BIT4;   //clr flag 4.4
    }
    else if((P4->IFG & BIT4) && connected){   //top button pressed and connected, then viewFriendScreen
        state.currentBoard = FRIEND;
        P4->IFG &= ~BIT4;   //clr flag 4.4
    }
    else if(P4->IFG & BIT5){    //right button pressed, increase brush size
        increaseBrush = true;
        P4->IFG &= ~BIT5;   //clr flag 4.5
    }
    else if(P4->IFG & BIT0){  //Touchpad press
        touchReceived = true;
        P4->IFG &= ~BIT0;
    }
    else if(P4->IFG & BIT3){ //joystick press -> reset brush to default
        state.currentBrush.color.c = DEFAULT_BRUSH_COLOR;
        state.currentBrush.size = DEFAULT_BRUSH_SIZE;
    }
    
    EndCriticalSection(cs_value);
}

void PORT5_IRQHandler(){
    uint16_t cs_value = StartCriticalSection();
    if((P5->IFG & BIT4) && !connected){     //if bottom button (b2) pressed and not connected yet
        isClient = true;
        connected = true;
        P5->IFG &= ~BIT4;   //clr flag 5.4
    }
    else if((P5->IFG & BIT4) && connected && (state.currentBoard != SELF)){   //bottom button pressed and connected, then viewOwnScreen
        state.currentBoard = SELF;
        P5->IFG &= ~BIT4;   //clr flag 5.4
    }
    else if((P5->IFG & BIT4) && connected && (state.currentBoard == SELF)){   //bottom button pressed, already on self, so UNDO
        state.undo++;
        P5->IFG &= ~BIT4;   //clr flag 5.4
    }
    else if(P5->IFG & BIT5){    //left button pressed, decrease brush size
        decreaseBrush = true;
        P5->IFG &= ~BIT5;   //clr flag 5.5
    }
    EndCriticalSection(cs_value);
}

void WaitInit(){
    initInterrupts();
//    while(!(isHost || isClient));
    G8RTOS_InitSemaphore(&cc3100, 1);
    G8RTOS_InitSemaphore(&lcd, 1);
    G8RTOS_InitSemaphore(&globalState, 1);
//    if(isHost){
//        InitHost();
//    }else if(isClient){
//        InitClient();
//    }
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
  LCD_Init(true);
  state.currentBrush.color.c = DEFAULT_BRUSH_COLOR;
  state.currentBrush.size = DEFAULT_BRUSH_SIZE;

//  G8RTOS_AddThread(Send, 6, sendDataName);   // *** add priorities ***
//  G8RTOS_AddThread(Receive, 5, receiveDataName);
  G8RTOS_AddThread(IdleThread, 254, idlethreadName);
  G8RTOS_AddThread(Draw, 3, drawName);
  G8RTOS_AddThread(ProcessTouch, 4, processTouchName);
  G8RTOS_AddThread(ReadJoystick, 7, joyName);

  G8RTOS_KillSelf();
}

void DrawBoard(){
  G8RTOS_WaitSemaphore(&lcd);
  LCD_Clear(BACK_COLOR);
  LCD_DrawRectangle(DRAWABLE_MIN_X, DRAWABLE_MAX_X, DRAWABLE_MIN_Y, DRAWABLE_MAX_Y, DRAW_COLOR);
  DrawInfo();
  G8RTOS_SignalSemaphore(&lcd);
}

void RedrawStrokes(){
  G8RTOS_WaitSemaphore(&globalState);
  BrushStroke_t * queue;
  uint16_t top;
  switch(state.currentBoard){
    case SELF:
      queue = &selfQueue.strokes[0];
      top = state.stackPos;
      break;
    case FRIEND:
      queue = &friendQueue.strokes[0];
      top = state.friendStackPos;
      break;
    default:
      G8RTOS_SignalSemaphore(&globalState);
      return;
  }
  G8RTOS_WaitSemaphore(&lcd);
  for(int i = 0; i < top; i++){
    DrawStroke(&queue[i]);
  }
  G8RTOS_SignalSemaphore(&lcd);
  G8RTOS_SignalSemaphore(&globalState);
}

void DrawStroke(BrushStroke_t * stroke){
  LCD_DrawRectangle(stroke->pos.x + DRAWABLE_MIN_X - (stroke->brush.size>>1),
                    stroke->pos.x + DRAWABLE_MIN_X + (stroke->brush.size>>1),
                    stroke->pos.y + DRAWABLE_MIN_Y - (stroke->brush.size>>1),
                    stroke->pos.y + DRAWABLE_MIN_Y + (stroke->brush.size>>1),
                    stroke->brush.color.c);
  //LCD_DrawCircle here
}

void SendStroke(BrushStroke_t * stroke){
  G8RTOS_WaitSemaphore(&cc3100);
  Packet_t send = {point,*stroke};
  SendData((uint8_t *)&send, state.ip, sizeof(Packet_t));
  G8RTOS_SignalSemaphore(&cc3100);
}

void Undo(){
  G8RTOS_WaitSemaphore(&cc3100);
  Packet_t send;
  send.header = undo;
  SendData((uint8_t *)&send, state.ip, sizeof(Packet_t));
  G8RTOS_SignalSemaphore(&cc3100);
}

void ReceiveStroke(BrushStroke_t * stroke){
  G8RTOS_WaitSemaphore(&cc3100);
  Packet_t receive;
  while(ReceiveData((uint8_t *)&receive, sizeof(Packet_t)) == NOTHING_RECEIVED){
    G8RTOS_SignalSemaphore(&cc3100);
    sleep(1);
    G8RTOS_WaitSemaphore(&cc3100);
  }
  G8RTOS_SignalSemaphore(&cc3100);
  if(receive.header == point){
    friendQueue.strokes[state.friendStackPos] = receive.stroke;
  }else{
    state.friendStackPos--;
  }
}

void DrawInfo(){
  BrushStroke_t current;
  current.brush = state.currentBrush;
  current.pos = (ScreenPos_t){0,0};
  DrawStroke(&current);
}

/*********************************************** Public Functions *********************************************************************/
