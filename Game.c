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
char accelName[] = "Accel";

StrokeQueue_t selfQueue;
StrokeQueue_t friendQueue;
/* Semaphores here */
semaphore_t cc3100;
semaphore_t lcd;
semaphore_t globalState;

uint16_t colorwheel[256] = {0xffff,0xfefe,0xfdfd,0xfcfc,0xfbfb,0xfafa,0xf9f9,0xf8f8,
                            0xf7f7,0xf6f6,0xf5f5,0xf4f4,0xf3f3,0xf2f2,0xf1f1,0xf0f0,
                            0xefef,0xeeee,0xeded,0xecec,0xebeb,0xeaea,0xe9e9,0xe8e8,
                            0xe7e7,0xe6e6,0xe5e5,0xe4e4,0xe3e3,0xe2e2,0xe1e1,0xe0e0,
                            0xdfdf,0xdede,0xdddd,0xdcdc,0xdbdb,0xdada,0xd9d9,0xd8d8,
                            0xd7d7,0xd6d6,0xd5d5,0xd4d4,0xd3d3,0xd2d2,0xd1d1,0xd0d0,
                            0xcfcf,0xcece,0xcdcd,0xcccc,0xcbcb,0xcaca,0xc9c9,0xc8c8,
                            0xc7c7,0xc6c6,0xc5c5,0xc4c4,0xc3c3,0xc2c2,0xc1c1,0xc0c0,
                            0xbfbf,0xbebe,0xbdbd,0xbcbc,0xbbbb,0xbaba,0xb9b9,0xb8b8,
                            0xb7b7,0xb6b6,0xb5b5,0xb4b4,0xb3b3,0xb2b2,0xb1b1,0xb0b0,
                            0xafaf,0xaeae,0xadad,0xacac,0xabab,0xaaaa,0xa9a9,0xa8a8,
                            0xa7a7,0xa6a6,0xa5a5,0xa4a4,0xa3a3,0xa2a2,0xa1a1,0xa0a0,
                            0x9f9f,0x9e9e,0x9d9d,0x9c9c,0x9b9b,0x9a9a,0x9999,0x9898,
                            0x9797,0x9696,0x9595,0x9494,0x9393,0x9292,0x9191,0x9090,
                            0x8f8f,0x8e8e,0x8d8d,0x8c8c,0x8b8b,0x8a8a,0x8989,0x8888,
                            0x8787,0x8686,0x8585,0x8484,0x8383,0x8282,0x8181,0x8080,
                            0x7f7f,0x7e7e,0x7d7d,0x7c7c,0x7b7b,0x7a7a,0x7979,0x7878,
                            0x7777,0x7676,0x7575,0x7474,0x7373,0x7272,0x7171,0x7070,
                            0x6f6f,0x6e6e,0x6d6d,0x6c6c,0x6b6b,0x6a6a,0x6969,0x6868,
                            0x6767,0x6666,0x6565,0x6464,0x6363,0x6262,0x6161,0x6060,
                            0x5f5f,0x5e5e,0x5d5d,0x5c5c,0x5b5b,0x5a5a,0x5959,0x5858,
                            0x5757,0x5656,0x5555,0x5454,0x5353,0x5252,0x5151,0x5050,
                            0x4f4f,0x4e4e,0x4d4d,0x4c4c,0x4b4b,0x4a4a,0x4949,0x4848,
                            0x4747,0x4646,0x4545,0x4444,0x4343,0x4242,0x4141,0x4040,
                            0x3f3f,0x3e3e,0x3d3d,0x3c3c,0x3b3b,0x3a3a,0x3939,0x3838,
                            0x3737,0x3636,0x3535,0x3434,0x3333,0x3232,0x3131,0x3030,
                            0x2f2f,0x2e2e,0x2d2d,0x2c2c,0x2b2b,0x2a2a,0x2929,0x2828,
                            0x2727,0x2626,0x2525,0x2424,0x2323,0x2222,0x2121,0x2020,
                            0x1f1f,0x1e1e,0x1d1d,0x1c1c,0x1b1b,0x1a1a,0x1919,0x1818,
                            0x1717,0x1616,0x1515,0x1414,0x1313,0x1212,0x1111,0x1010,
                            0xf0f,0xe0e,0xd0d,0xc0c,0xb0b,0xa0a,0x909,0x808,0x707,
                            0x606,0x505,0x404,0x303,0x202,0x101,0x0};

/*********************************************** Common Threads *********************************************************************/

void Send(){
  while(1){
     if(state.undo){
         Undo();
         state.undo--;
     }else if(state.stackPos > state.lastSentStroke){
         SendStroke(&selfQueue.strokes[state.lastSentStroke]);
         state.lastSentStroke++;
     }
    sleep(10);
  }
}

void Receive(){
  while(1){
    if(state.friendStackPos < MAX_STROKES){
        ReceiveStroke();
    }
    sleep(8);
  }
}

bool touchReceived;
void ProcessTouch(){
  uint16_t touchX;
  uint16_t touchY;
//  uint16_t lastX;
//  uint16_t lastY;
  while(1){
//    if(touchReceived){
      if(state.currentBoard == SELF){
      touchX = TP_ReadX();
      touchY = TP_ReadY();
//      touchReceived = false;
        G8RTOS_WaitSemaphore(&globalState);
        if(touchX > DRAWABLE_MIN_X && touchX < DRAWABLE_MAX_X &&
           touchY > DRAWABLE_MIN_Y && touchY < DRAWABLE_MAX_Y){
//          if(abs((int16_t)touchX-lastX) > MIN_DIST || abs((int16_t)touchX-lastY) > MIN_DIST){
//            lastX = touchX;
//            lastY = touchY;
            selfQueue.strokes[state.stackPos].pos.x = touchX - DRAWABLE_MIN_X;
            selfQueue.strokes[state.stackPos].pos.y = touchY - DRAWABLE_MIN_Y;
            selfQueue.strokes[state.stackPos].brush.color = state.currentBrush.color;
            selfQueue.strokes[state.stackPos].brush.size = state.currentBrush.size;
            if(state.stackPos < MAX_STROKES){
                state.stackPos++;
            }
//          }
        }
        G8RTOS_SignalSemaphore(&globalState);
    }
    sleep(10);
  }
}

void ReadJoystick(){
    int16_t joyX;
    int16_t joyY;
    while(1){
        if(state.currentBoard == SELF){
            GetJoystickCoordinates(&joyX, &joyY);
            if(joyX > 8000 || joyY > 8000 || joyX < -6800 || joyY < -8000){
                joyY = joyY < 0 ? -joyY : joyY;
                state.currentBrush.color.c = (joyX>>8) | (joyY>>12);
            }
        }
        sleep(10);
    }
}


void Draw(){
  uint8_t prevBoard = (uint8_t) -1;
  uint16_t lastStackPosition = 0;
  uint16_t lastFriendStackPosition = FRIEND_OFFSET;
  Brush_t prevBrush;
  while(1){
      if(prevBoard == state.currentBoard && state.currentBoard == SELF){
        if(state.stackPos > lastStackPosition){
          for(; state.stackPos > lastStackPosition; lastStackPosition++){
            DrawStroke(&selfQueue.strokes[lastStackPosition]);
          }
        }else if(state.stackPos < lastStackPosition){
            while(state.stackPos < lastStackPosition){
                lastStackPosition--;
                selfQueue.strokes[lastStackPosition].brush.color.c = DRAW_COLOR_INDEX;
                DrawStroke(&selfQueue.strokes[lastStackPosition]);
                RedrawStrokesNear(selfQueue.strokes[lastStackPosition].pos, selfQueue.strokes[lastStackPosition].brush.size);
            }
//           DrawBoard();
//            RedrawStrokes();
        }
      }else if(prevBoard == state.currentBoard && state.currentBoard == FRIEND){
        if(state.friendStackPos > lastFriendStackPosition){
          for(; state.friendStackPos > lastFriendStackPosition; lastFriendStackPosition++){
            DrawStroke(&friendQueue.strokes[lastFriendStackPosition]);
          }
        }else if(state.friendStackPos < lastFriendStackPosition){
            while(state.friendStackPos < lastFriendStackPosition){
                lastFriendStackPosition--;
                friendQueue.strokes[lastFriendStackPosition].brush.color.c = DRAW_COLOR_INDEX;
                DrawStroke(&friendQueue.strokes[lastFriendStackPosition]);
                RedrawStrokesNear(friendQueue.strokes[lastFriendStackPosition].pos, friendQueue.strokes[lastFriendStackPosition].brush.size<<2);
            }
//          DrawBoard();
//            RedrawStrokes();
        }
      }else if(prevBoard != state.currentBoard){
        DrawBoard();
        RedrawStrokes();
        prevBoard = state.currentBoard;
        lastFriendStackPosition = state.friendStackPos;
        lastStackPosition = state.stackPos;
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
     //Button Init 4.3 (joystick press) -> reset brush to default
     //NOTE: touchpad interrupt (4.0) is initialized in LCDLib, but is currently disabled b/c LCD_init(false)
     P4->DIR &= ~(BIT3 | BIT4 | BIT5);   //set as input
     P4->IFG &= ~(BIT3 | BIT4 | BIT5);   //P4.4 IFG cleared
     P4->IE  |= (BIT3 | BIT4 | BIT5);     //Enable interrupt on P4.4
     P4->IES |= (BIT3 | BIT4 | BIT5);    //falling edge triggered
     P4->REN |= (BIT3 | BIT4 | BIT5);    //pull-up resistor
     P4->OUT |= (BIT3 | BIT4 | BIT5);    //Sets res to pull-up

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
void PORT4_IRQHandler(){
    uint16_t cs_value = StartCriticalSection();
    if((P4->IFG & BIT4) && !connected){     //if top button (b0) pressed and not connected yet
        isHost = true;
        connected = true;
        P4->IFG &= ~BIT4;   //clr flag 4.4
    }
    else if((P4->IFG & BIT4) && connected){   //top button pressed and connected, then viewFriendScreen
        state.currentBoard = FRIEND;
        P4->IE &= ~(BIT3 | BIT4 | BIT5);    //disable interrupts that only need to be used when viewing ownScreen (top, right, joystick button)
        P5->IE &= ~BIT5;    // ^^^ (disable left button interrupt)

        P4->IFG &= ~BIT4;   //clr flag 4.4
    }
    else if((P4->IFG & BIT5) && state.currentBoard == SELF){    //right button pressed, increase brush size
        if(state.currentBrush.size < MAX_BRUSH_SIZE){
            state.currentBrush.size += 2;
        }
        P4->IFG &= ~BIT5;   //clr flag 4.5
    }
//    else if((P4->IFG & BIT0) && state.currentBoard == SELF){  //Touchpad press
//        touchReceived = true;
//        P4->IFG &= ~BIT0;
//    }
    else if((P4->IFG & BIT3) && state.currentBoard == SELF){ //joystick press -> reset brush to default
        state.currentBrush.color.c = DEFAULT_BRUSH_COLOR_INDEX;
        state.currentBrush.size = DEFAULT_BRUSH_SIZE;
        P4->IFG &= ~BIT3;
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
        P4->IE  |= (BIT3 | BIT4 | BIT5);    //enable interrupts that are to be used only when viewingOwnScreen (top, right, joystick button)
        P5->IE |= BIT5;     //^^^ (left button)
        P4->IFG &= ~(BIT3 | BIT4 | BIT5);
        P5->IFG &= ~BIT5;

        P5->IFG &= ~BIT4;   //clr flag 5.4
    }
    else if((P5->IFG & BIT4) && connected && (state.currentBoard == SELF)){   //bottom button pressed, already on self, so UNDO
        if(state.stackPos > 0){
            state.undo++;
            state.stackPos--;
        }
        P5->IFG &= ~BIT4;   //clr flag 5.4
    }
    else if((P5->IFG & BIT5) && state.currentBoard == SELF){    //left button pressed, decrease brush size
        if(state.currentBrush.size > MIN_BRUSH_SIZE){
            state.currentBrush.size -= 2;
        }
        P5->IFG &= ~BIT5;   //clr flag 5.5
    }
    EndCriticalSection(cs_value);
}

void WaitInit(){
    initInterrupts();
    LCD_Init(false);
    while(!(isHost || isClient));
    G8RTOS_InitSemaphore(&cc3100, 1);
    G8RTOS_InitSemaphore(&lcd, 1);
    G8RTOS_InitSemaphore(&globalState, 1);
    if(isHost){
        InitHost();
    }else if(isClient){
        InitClient();
    }
    CreateThreadsAndStart();

}

void Accel(){
    int16_t x;
    int16_t y;
    while(1){
        G8RTOS_WaitSemaphore(&cc3100);
        bmi160_read_accel_x(&x);
        bmi160_read_accel_y(&y);
        G8RTOS_SignalSemaphore(&cc3100);
        if(abs(x) > 14000 && abs(y) > 14000 ){
            state.lastSentStroke = 0;
            state.stackPos = 0;
            G8RTOS_WaitSemaphore(&cc3100);
            Packet_t send;
            send.header = clear;
            SendData((uint8_t *)&send, state.ip, sizeof(Packet_t));
            G8RTOS_SignalSemaphore(&cc3100);
        }
        sleep(6);
    }
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
  state.ip = HOST_IP_ADDR;

  uint8_t temp;
  while(ReceiveData(&temp, sizeof(uint8_t)) == NOTHING_RECEIVED);

  P2->DIR |= BLUE_LED;  //p1.0 set to output
  BITBAND_PERI(P2->OUT, 2) = 1;  //toggle led on
}

void CreateThreadsAndStart(){
  state.currentBrush.color.c = DEFAULT_BRUSH_COLOR_INDEX;
  state.currentBrush.size = DEFAULT_BRUSH_SIZE;
  state.friendStackPos = FRIEND_OFFSET;

  G8RTOS_AddThread(Send, 6, sendDataName);   // *** add priorities ***
  G8RTOS_AddThread(Receive, 5, receiveDataName);
  G8RTOS_AddThread(IdleThread, 254, idlethreadName);
  G8RTOS_AddThread(Draw, 3, drawName);
  G8RTOS_AddThread(ProcessTouch, 4, processTouchName);
  G8RTOS_AddThread(ReadJoystick, 7, joyName);
  G8RTOS_AddThread(Accel, 8, accelName);

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
  uint16_t current = 0;
  switch(state.currentBoard){
    case SELF:
      queue = &selfQueue.strokes[0];
      top = state.stackPos;
      break;
    case FRIEND:
      queue = &friendQueue.strokes[0];
      top = state.friendStackPos;
      current = FRIEND_OFFSET;
      break;
    default:
      G8RTOS_SignalSemaphore(&globalState);
      return;
  }
  G8RTOS_WaitSemaphore(&lcd);
  for(; current < top; current++){
    DrawStroke(&queue[current]);
  }
  G8RTOS_SignalSemaphore(&lcd);
  G8RTOS_SignalSemaphore(&globalState);
}

void DrawStroke(BrushStroke_t * stroke){
  uint16_t xStart = stroke->pos.x + DRAWABLE_MIN_X - (stroke->brush.size>>1);
  uint16_t xEnd = stroke->pos.x + DRAWABLE_MIN_X + (stroke->brush.size>>1);
  uint16_t yStart = stroke->pos.y + DRAWABLE_MIN_Y - (stroke->brush.size>>1);
  uint16_t yEnd = stroke->pos.y + DRAWABLE_MIN_Y + (stroke->brush.size>>1);
  if(xStart < DRAWABLE_MIN_X || xStart > 10000){
      xStart = DRAWABLE_MIN_X;
  }
  if(xEnd > DRAWABLE_MAX_X){
      xEnd = DRAWABLE_MAX_X;
  }
  if(yStart < DRAWABLE_MIN_Y || yStart > 10000){
      yStart = DRAWABLE_MIN_Y;
  }
  if(yEnd > DRAWABLE_MAX_Y){
      yEnd = DRAWABLE_MAX_Y;
  }
  LCD_DrawRectangle(xStart, xEnd, yStart, yEnd,
                    colorwheel[stroke->brush.color.c]);
  //LCD_DrawCircle here
}

void RedrawStrokesNear(ScreenPos_t pos, uint8_t dist){
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
      if(abs(queue[i].pos.x-pos.x) < dist && abs(queue[i].pos.y-pos.y) < dist)
          DrawStroke(&queue[i]);
    }
    G8RTOS_SignalSemaphore(&lcd);
    G8RTOS_SignalSemaphore(&globalState);
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

void ReceiveStroke(){
  G8RTOS_WaitSemaphore(&cc3100);
  Packet_t receive;
  if(ReceiveData((uint8_t *)&receive, sizeof(Packet_t)) == NOTHING_RECEIVED){
      G8RTOS_SignalSemaphore(&cc3100);
  }else{
      G8RTOS_SignalSemaphore(&cc3100);
      if(receive.header == point){
        friendQueue.strokes[state.friendStackPos] = receive.stroke;
        state.friendStackPos++;
      }else if(receive.header == undo){
          if(state.friendStackPos > FRIEND_OFFSET){
              state.friendStackPos--;
          }
      }else if(receive.header == clear){
          state.friendStackPos = FRIEND_OFFSET;
      }
  }
}

void DrawInfo(){
  char brushsize[16] = "Size: 8";
  if(state.currentBoard == SELF){
      LCD_DrawRectangle(0, 20, 0, 20, LCD_WHITE);
      LCD_DrawRectangle(6, 14, 6, 14, colorwheel[state.currentBrush.color.c]);
      LCD_DrawRectangle(0, 64, DRAWABLE_MAX_Y, 240, LCD_BLACK);
      sprintf(brushsize, "Size: %d", state.currentBrush.size);
      LCD_Text(0, 220, (uint8_t*)brushsize, LCD_WHITE);
  }
}

/*********************************************** Public Functions *********************************************************************/
