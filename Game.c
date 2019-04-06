#include "Game.h"

/*********************************************** Global Vars *********************************************************************/
GameState_t gameState;
semaphore_t cc3100;

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame(){
  G8RTOS_InitSemaphore(&cc3100, 1);
  initCC3100(Client);

  G8RTOS_InitSemaphore(&lcd, 1);
  G8RTOS_InitSemaphore(&cc3100, 1);

  //init specific player info for client
  gameState.player.IP_address = getLocalIP();
  gameState.player.displacement = 0;
  gameState.player.playerNumber = Client;  //0 = client, 1 = host
  gameState.player.ready = true;
  gameState.player.joined = false;
  gameState.player.acknowledge = false;

  G8RTOS_WaitSemaphore(&cc3100);
  SendData((uint8_t *)&gameState.player, HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t));
  G8RTOS_SignalSemaphore(&cc3100);

  G8RTOS_WaitSemaphore(&cc3100);
  while(ReceiveData((uint8_t *)&gameState.player, sizeof(SpecificPlayerInfo_t)) == NOTHING_RECEIVED);
  G8RTOS_SignalSemaphore(&cc3100);

  P1->DIR |= BIT0;  //p1.0 set to output
  BITBAND_PERI(P1->OUT, 0) ^= 1;  //toggle led on
  
  LCD_Init(false);
  LCD_DrawRectangle(ARENA_MIN_X, ARENA_MAX_X, ARENA_MIN_X, ARENA_MAX_Y,LCD_BLACK);

  //change priorities later
  // G8RTOS_AddThread(ReadJoystickClient, 4, NULL);
  // G8RTOS_AddThread(SendDataToHost, 8, NULL);
  // G8RTOS_AddThread(ReceiveDataFromHost, 6, NULL);
  // G8RTOS_AddThread(DrawObjects, 5, NULL);
  // G8RTOS_AddThread(MoveLEDs, 250, NULL);
  // G8RTOS_AddThread(Idle, 255, NULL);
  
  // G8RTOS_KillSelf();
  
  while(1);
}

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost(){

}

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost(){

}

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient(){
  while(1){

  }
}

/*
 * End of game for the client
 */
void EndOfGameClient(){

}
/*********************************************** Client Threads *********************************************************************/


/*********************************************** Host Threads *********************************************************************/
/*
 * Thread for the host to create a game
 */
void CreateGame(){
  G8RTOS_AddThread(IdleThread,255,NULL);
  G8RTOS_InitSemaphore(&cc3100, 1);

  gameState.players[Host]= (GeneralPlayerInfo_t){PADDLE_X_CENTER, PLAYER_RED, BOTTOM};
  gameState.players[Client]= (GeneralPlayerInfo_t){PADDLE_X_CENTER, PLAYER_BLUE, TOP};

  G8RTOS_WaitSemaphore(&cc3100);
  initCC3100(Host);

  while(ReceiveData((uint8_t * )&gameState.player, sizeof(SpecificPlayerInfo_t)) == NOTHING_RECEIVED);
  gameState.player.joined = true;
  gameState.player.acknowledge = true;
  SendData((uint8_t * ) &gameState.player, gameState.player.IP_address, sizeof(gameState.player));
  G8RTOS_SignalSemaphore(&cc3100);

  P1->DIR |= BIT0;  //p1.0 set to output
  BITBAND_PERI(P1->OUT, 0) ^= 1;  //toggle led on

  LCD_Init(false);
  LCD_DrawRectangle(ARENA_MIN_X, ARENA_MAX_X, ARENA_MIN_X, ARENA_MAX_Y,LCD_BLACK);
  while(1);
}

/*
 * Thread that sends game state to client
 */
void SendDataToClient(){

}

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient(){

}

/*
 * Generate Ball thread
 */
void GenerateBall(){

}

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost(){

}

/*
 * Thread to move a single ball
 */
void MoveBall(){

}

/*
 * End of game for the host
 */
void EndOfGameHost(){

}

/*********************************************** Host Threads *********************************************************************/


/*********************************************** Common Threads *********************************************************************/
/*
 * Idle thread
 */
void IdleThread(){
  while(1);
}

/*
 * Thread to draw all the objects in the game
 */
void DrawObjects(){
  GameState_t gameStatePrev = gameState;
  while(1){
    for(int i = 0; i < MAX_NUM_OF_PLAYERS; i++){
      if(gameStatePrev.players[i].position != gameState.players[i].position){

        gameStatePrev.players[i].position = gameState.players[i].position;
      }
    }
    for(int i = 0; i < MAX_NUM_OF_BALLS; i++){
      if(gameState.balls[i].alive){
        if(gameStatePrev.balls[i].currentCenterX != gameState.balls[i].currentCenterX){
          
          gameStatePrev.balls[i].currentCenterX = gameState.balls[i].currentCenterX;
        }
        if(gameStatePrev.balls[i].currentCenterY != gameState.balls[i].currentCenterY){
          
          gameStatePrev.balls[i].currentCenterY != gameState.balls[i].currentCenterY;
        }
      }else if(gameStatePrev.balls[i].alive != gameState.balls[i].alive){
        
        gameStatePrev.balls[i].alive = gameState.balls[i].alive;
      }
    }
    sleep(20);
  }
}

/*
 * Thread to update LEDs based on score
 */
void MoveLEDs(){

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
        G8RTOS_AddThread(CreateGame,1,NULL);
    }else if(isClient){
        G8RTOS_AddThread(JoinGame,1,NULL);
    }
    G8RTOS_KillSelf();
}

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/
/*
 * Returns either Host or Client depending on button press
 */
playerType GetPlayerRole(){

}

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t * player){

}

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer){

}

/*
 * Function updates ball position on screen
 */
void UpdateBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall, uint16_t outColor){

}

/*
 * Initializes and prints initial game state
 */
void InitBoardState(){

}

/*********************************************** Public Functions *********************************************************************/
