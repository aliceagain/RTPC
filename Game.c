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

uint16_t colorwheel[256] = {0x8000,0x8324,0x8647,0x896a,0x8c8b,0x8fab,0x92c7,0x95e1,
                            0x98f8,0x9c0b,0x9f19,0xa223,0xa527,0xa826,0xab1f,0xae10,
                            0xb0fb,0xb3de,0xb6b9,0xb98c,0xbc56,0xbf17,0xc1cd,0xc47a,
                            0xc71c,0xc9b3,0xcc3f,0xcebf,0xd133,0xd39a,0xd5f5,0xd842,
                            0xda82,0xdcb3,0xded7,0xe0eb,0xe2f1,0xe4e8,0xe6cf,0xe8a6,
                            0xea6d,0xec23,0xedc9,0xef5e,0xf0e2,0xf254,0xf3b5,0xf504,
                            0xf641,0xf76b,0xf884,0xf989,0xfa7c,0xfb5c,0xfc29,0xfce3,
                            0xfd89,0xfe1d,0xfe9c,0xff09,0xff61,0xffa6,0xffd8,0xfff5,
                            0xffff,0xfff5,0xffd8,0xffa6,0xff61,0xff09,0xfe9c,0xfe1d,
                            0xfd89,0xfce3,0xfc29,0xfb5c,0xfa7c,0xf989,0xf884,0xf76b,
                            0xf641,0xf504,0xf3b5,0xf254,0xf0e2,0xef5e,0xedc9,0xec23,
                            0xea6d,0xe8a6,0xe6cf,0xe4e8,0xe2f1,0xe0eb,0xded7,0xdcb3,
                            0xda82,0xd842,0xd5f5,0xd39a,0xd133,0xcebf,0xcc3f,0xc9b3,
                            0xc71c,0xc47a,0xc1cd,0xbf17,0xbc56,0xb98c,0xb6b9,0xb3de,
                            0xb0fb,0xae10,0xab1f,0xa826,0xa527,0xa223,0x9f19,0x9c0b,
                            0x98f8,0x95e1,0x92c7,0x8fab,0x8c8b,0x896a,0x8647,0x8324,
                            0x8000,0x7cdb,0x79b8,0x7695,0x7374,0x7054,0x6d38,0x6a1e,
                            0x6707,0x63f4,0x60e6,0x5ddc,0x5ad8,0x57d9,0x54e0,0x51ef,
                            0x4f04,0x4c21,0x4946,0x4673,0x43a9,0x40e8,0x3e32,0x3b85,
                            0x38e3,0x364c,0x33c0,0x3140,0x2ecc,0x2c65,0x2a0a,0x27bd,
                            0x257d,0x234c,0x2128,0x1f14,0x1d0e,0x1b17,0x1930,0x1759,
                            0x1592,0x13dc,0x1236,0x10a1,0xf1d,0xdab,0xc4a,0xafb,
                            0x9be,0x894,0x77b,0x676,0x583,0x4a3,0x3d6,0x31c,
                            0x276,0x1e2,0x163,0xf6,0x9e,0x59,0x27,0xa,
                            0x0,0xa,0x27,0x59,0x9e,0xf6,0x163,0x1e2,
                            0x276,0x31c,0x3d6,0x4a3,0x583,0x676,0x77b,0x894,
                            0x9be,0xafb,0xc4a,0xdab,0xf1d,0x10a1,0x1236,0x13dc,
                            0x1592,0x1759,0x1930,0x1b17,0x1d0e,0x1f14,0x2128,0x234c,
                            0x257d,0x27bd,0x2a0a,0x2c65,0x2ecc,0x3140,0x33c0,0x364c,
                            0x38e3,0x3b85,0x3e32,0x40e8,0x43a9,0x4673,0x4946,0x4c21,
                            0x4f04,0x51ef,0x54e0,0x57d9,0x5ad8,0x5ddc,0x60e6,0x63f4,
                            0x6707,0x6a1e,0x6d38,0x7054,0x7374,0x7695,0x79b8,0x7cdb};

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
        bmi160_read_accel_x(&x);
        bmi160_read_accel_y(&y);
        if(abs(x) > 14000 && abs(y) > 14000 ){
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
  if(xStart < DRAWABLE_MIN_X){
      xStart = DRAWABLE_MIN_X;
  }
  if(xEnd > DRAWABLE_MAX_X){
      xEnd = DRAWABLE_MAX_X;
  }
  if(yStart < DRAWABLE_MIN_Y){
      yStart = DRAWABLE_MIN_Y;
  }
  if(yEnd > DRAWABLE_MAX_X){
      yEnd = DRAWABLE_MAX_X;
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
      LCD_DrawRectangle(10, 18, 10, 18, colorwheel[state.currentBrush.color.c]);
      LCD_DrawRectangle(0, 64, DRAWABLE_MAX_Y, 240, LCD_BLACK);
      sprintf(brushsize, "Size: %d", state.currentBrush.size);
      LCD_Text(0, 220, (uint8_t*)brushsize, LCD_WHITE);
  }
}

/*********************************************** Public Functions *********************************************************************/
