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

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame(){
  G8RTOS_InitSemaphore(&cc3100, 1);
  G8RTOS_InitSemaphore(&lcd, 1);
  G8RTOS_InitSemaphore(&player, 1);
  initCC3100(Client);


  //init specific player info for client
  gameState.player.IP_address = getLocalIP();
  gameState.player.displacementX = 0;
  gameState.player.displacementY = 0;
  gameState.player.playerNumber = CLIENT;  //0 = client, 1 = host
  gameState.player.ready = true;
  gameState.player.joined = false;
  gameState.player.acknowledge = false;

  G8RTOS_WaitSemaphore(&cc3100);
  SendData((uint8_t *)&gameState.player, HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t));
  G8RTOS_SignalSemaphore(&cc3100);

  G8RTOS_WaitSemaphore(&cc3100);
  while(ReceiveData((uint8_t *)&gameState, sizeof(GameState_t)) == NOTHING_RECEIVED);
  G8RTOS_SignalSemaphore(&cc3100);

  P2->DIR |= BLUE_LED;  //p1.0 set to output
  BITBAND_PERI(P2->OUT, 2) = 1;  //toggle led on

  InitBoardState();

  //change priorities later

  G8RTOS_AddThread(DrawObjects, 1, drawobjectsName);
  G8RTOS_AddThread(ReadJoystickClient, 2, readjoystickName);
  G8RTOS_AddThread(SendDataToHost, 5, senddatatName);
  G8RTOS_AddThread(ReceiveDataFromHost, 4, receivedataName);
  G8RTOS_AddThread(IdleThread, 254, idlethreadName);
  
  G8RTOS_KillSelf();
}

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost(){
  int16_t inpacket[12];
  while(1){
    G8RTOS_WaitSemaphore(&cc3100);
    while(ReceiveData((uint8_t *)&inpacket, sizeof(int16_t) * 12) == NOTHING_RECEIVED){
      G8RTOS_SignalSemaphore(&cc3100);  

      sleep(1);
      G8RTOS_WaitSemaphore(&cc3100);
    }
    G8RTOS_SignalSemaphore(&cc3100);

    G8RTOS_WaitSemaphore(&player);
    gameState.players[0].headX = inpacket[0];
    gameState.players[0].headY = inpacket[1];
    gameState.players[0].tailX = inpacket[2];
    gameState.players[0].tailY = inpacket[3];
    gameState.players[1].headX = inpacket[4];
    gameState.players[1].headY = inpacket[5];
    gameState.players[1].tailX = inpacket[6];
    gameState.players[1].tailY = inpacket[7];
    gameState.snack.currentCenterX = inpacket[8];
    gameState.snack.currentCenterY = inpacket[9];
    gameState.winner = inpacket[10];
    gameState.gameDone = inpacket[11];
    G8RTOS_SignalSemaphore(&player);


    if(gameState.gameDone == true){
      G8RTOS_AddThread(EndOfGameClient, 0, endofgameName);
    }
    sleep(2);
  }
}

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost(){
  while(1){
    G8RTOS_WaitSemaphore(&cc3100);
    SendData((uint8_t *)&gameState.player.displacementX, HOST_IP_ADDR, sizeof(int16_t)*2);
    G8RTOS_SignalSemaphore(&cc3100);
    sleep(2);
  }
}

/*********************************************** Client Threads *********************************************************************/


/*********************************************** Host Threads *********************************************************************/
/*
 * Thread for the host to create a game
 */
void CreateGame(){
  G8RTOS_InitSemaphore(&cc3100, 1);
  G8RTOS_InitSemaphore(&lcd, 1);
  G8RTOS_InitSemaphore(&player, 1);
  G8RTOS_InitFIFO(HOST);
  G8RTOS_InitFIFO(CLIENT);
  spawnDot();
  gameState.players[HOST]= (GeneralPlayerInfo_t){100, 100, 0, 0, LCD_RED, 1, -1};
  gameState.players[CLIENT]= (GeneralPlayerInfo_t){200, 200, 0, 0, LCD_BLUE, 1, -1};
  gameState.winner = 0;
  gameState.gameDone = false;
  writeFIFO(HOST, gameState.players[HOST].headX);
  writeFIFO(HOST, gameState.players[HOST].headY);
  writeFIFO(HOST, gameState.players[CLIENT].headX);
  writeFIFO(HOST, gameState.players[CLIENT].headY);
  G8RTOS_WaitSemaphore(&cc3100);
  initCC3100(Host);

  while(ReceiveData((uint8_t * )&gameState.player, sizeof(SpecificPlayerInfo_t)) == NOTHING_RECEIVED);
  gameState.player.joined = true;
  gameState.player.acknowledge = true;
  SendData((uint8_t * ) &gameState, gameState.player.IP_address, sizeof(gameState));
  G8RTOS_SignalSemaphore(&cc3100);

  P2->DIR |= RED_LED;  //p1.0 set to output
  BITBAND_PERI(P2->OUT, 0) = 1;  //toggle led on

  InitBoardState();

  //change priorities later
  G8RTOS_AddThread(DrawObjects, 1, drawobjectsName);
  G8RTOS_AddThread(ReadJoystickHost, 4, readjoystickName);
  G8RTOS_AddThread(SendDataToClient, 2, senddatatName);
  G8RTOS_AddThread(ReceiveDataFromClient, 3, receivedataName);
  G8RTOS_AddThread(IdleThread,254,idlethreadName);


  G8RTOS_KillSelf();

  while(1);
}


/*********************************************** Host Threads *********************************************************************/


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
        if((lastDrawnStroke+1)%16 == lastSentStroke){
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
    if(isHost){
        InitHost();
    }else if(isClient){
        InitClient();
    }
    CreateThreadsAndStart();
}

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

void InitHost(){}

void InitClient(){}

void DrawBoard(){}

void RedrawStrokes(){}

void DrawStroke(){}

/*********************************************** Public Functions *********************************************************************/
