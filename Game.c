#include "Game.h"

/*********************************************** Global Vars *********************************************************************/
GameState_t gameState;

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame(){
  G8RTOS_InitSemaphore(&cc3100, 1);
  G8RTOS_InitSemaphore(&lcd, 1);
  initCC3100(Client);

  G8RTOS_InitSemaphore(&cc3100, 1);

  int16_t xcoord, ycoord; //just used to initialize joystick displacement (y not necessary)
  GetJoystickCoordinates(&xcoord, &ycoord);

  //init specific player info for client
  gameState.player.IP_address = getLocalIP();
  gameState.player.displacement = xcoord;
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

  P2->DIR |= BLUE_LED;  //p1.0 set to output
  BITBAND_PERI(P2->OUT, 2) = 1;  //toggle led on
  
  G8RTOS_WaitSemaphore(&lcd);
  LCD_Init(false);
  LCD_Clear(LCD_GRAY);
  LCD_DrawRectangle(ARENA_MIN_X, ARENA_MAX_X, ARENA_MIN_X, ARENA_MAX_Y,BACK_COLOR);
  G8RTOS_SignalSemaphore(&lcd);

  //change priorities later
  G8RTOS_AddThread(ReadJoystickClient, 4, NULL);
  G8RTOS_AddThread(SendDataToHost, 8, NULL);
  G8RTOS_AddThread(ReceiveDataFromHost, 6, NULL);
  G8RTOS_AddThread(DrawObjects, 5, NULL);
  G8RTOS_AddThread(MoveLEDs, 250, NULL);
  G8RTOS_AddThread(Idle, 255, NULL);
  
  G8RTOS_KillSelf();
  
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
  int16_t xcoord, ycoord; //ycoord not needed
  G8RTOS_InitFIFO(JOYSTICK_CLIENTFIFO);
  int16_t  
  while(1){
    GetJoystickCoordinates(&xcoord, &ycoord);   //read x coord;
    
    sleep(10);
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
  G8RTOS_InitSemaphore(&lcd, 1);

  gameState.players[Host]= (GeneralPlayerInfo_t){PADDLE_X_CENTER, PLAYER_RED, BOTTOM};
  gameState.players[Client]= (GeneralPlayerInfo_t){PADDLE_X_CENTER, PLAYER_BLUE, TOP};

  G8RTOS_WaitSemaphore(&cc3100);
  initCC3100(Host);

  while(ReceiveData((uint8_t * )&gameState.player, sizeof(SpecificPlayerInfo_t)) == NOTHING_RECEIVED);
  gameState.player.joined = true;
  gameState.player.acknowledge = true;
  SendData((uint8_t * ) &gameState.player, gameState.player.IP_address, sizeof(gameState.player));
  G8RTOS_SignalSemaphore(&cc3100);

  P2->DIR |= RED_LED;  //p1.0 set to output
  BITBAND_PERI(P2->OUT, 0) = 1;  //toggle led on

  G8RTOS_WaitSemaphore(&lcd);
  LCD_Init(false);
  LCD_Clear(LCD_GRAY);
  LCD_DrawRectangle(ARENA_MIN_X, ARENA_MAX_X, ARENA_MIN_X, ARENA_MAX_Y,LCD_BLACK);
  G8RTOS_SignalSemaphore(&lcd);

  //change priorities later
  G8RTOS_AddThread(GenerateBall, 0, NULL);
  G8RTOS_AddThread(DrawObjects, 0, NULL);
  G8RTOS_AddThread(ReadJoystickHost, 0, NULL);
  G8RTOS_AddThread(SendDataToClient, 0, NULL);
  G8RTOS_AddThread(ReceiveDataFromClient, 0, NULL);
  G8RTOS_AddThread(MoveLEDs, 0, NULL)
  G8RTOS_AddThread(Idle, 255, NULL);

  G8RTOS_KillSelf();

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
  while(1){
    int16_t tempDisplacement;
    G8RTOS_WaitSemaphore(&cc3100);
    while(ReceiveData((uint8_t * )&tempDisplacement, sizeof(int16_t)) == NOTHING_RECEIVED){
      G8RTOS_SignalSemaphore(&lcd);
      sleep(1);
      G8RTOS_WaitSemaphore(&cc3100);
    }
    G8RTOS_SignalSemaphore(&lcd);
    gameState.players[Client].currentCenter += tempDisplacement;
    if(gameState.players[Client].currentCenter > HORIZ_CENTER_MAX_PL){
      gameState.players[Client].currentCenter = HORIZ_CENTER_MAX_PL
    }else if(gameState.players[Client].currentCenter < HORIZ_CENTER_MIN_PL){
      gameState.players[Client].currentCenter = HORIZ_CENTER_MIN_PL
    }
    sleep(2);
  }
}

/*
 * Generate Ball thread
 */
void GenerateBall(){
  while(1){
    if(gameState.numberOfBalls < MAX_NUM_OF_BALLS){
      G8RTOS_AddThread(MoveBall,0,NULL);
      numberOfBalls++;
    }
    sleep(100*gameState.numberOfBalls);
  }
}

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost(){
  int16_t xcoord, ycoord; //ycoord not needed
  while(1){
    GetJoystickCoordinates(&xcoord, &ycoord);   //read x coord;
    //TODO: Turn ADC value into something realistic for movement.
    sleep(10);
    gameState.players[Host].currentCenter += xcoord;
  }  
}

/*
 * Thread to move a single ball
 */
void MoveBall(){
  Ball_t * ball = null;
  for(int i = 0; i < MAX_NUM_OF_BALLS; i++){
      if(!gameState.balls[i].alive){
        ball = i;
      }
  }
  if(!ball){
    G8RTOS_KillSelf();
  }
  ball->currentCenterX = (SystemTime-&ball>>2+gameState.players[Client].currentCenter)%HORIZ_CENTER_MAX_BALL + HORIZ_CENTER_MIN_BALL;
  ball->currentCenterY = (SystemTime-&ball>>2+gameState.players[Host].currentCenter)%VERT_CENTER_MAX_BALL + VERT_CENTER_MIN_BALL;
  ball->currentVelocityX = (SystemTime-&ball>>2+gameState.players[Host].currentCenter)%MAX_BALL_SPEED;
  ball->currentVelocityY = (SystemTime-&ball>>2+gameState.players[Client].currentCenter)%MAX_BALL_SPEED;
  ball->color = ballColor[SystemTime>>3%8];
  ball->alive = true;
  while(1){
    //TODO: MOVE BALLS
  }
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
  GameState_t gameStatePrev;
  while(1){
    for(int i = 0; i < MAX_NUM_OF_PLAYERS; i++){
      if(gameStatePrev.players[i].position != gameState.players[i].position){
        int16_t posDiff = gameState.players[i].position - gameStatePrev.players[i].position;
        if(posDiff > 0 && posDiff < PADDLE_LEN_D2 ){
          if(i == 0){
            LCD_DrawRectangle(gameState.players[i].position+PADDLE_LEN_D2,
                              gameStatePrev.players[i].position+PADDLE_LEN_D2,
                              HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                              HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                              BACK_COLOR);
            LCD_DrawRectangle(gameState.players[i].position-PADDLE_LEN_D2,
                              gameStatePrev.players[i].position-PADDLE_LEN_D2,
                              HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                              HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                              PLAYER_RED);
          }else{
            LCD_DrawRectangle(gameState.players[i].position+PADDLE_LEN_D2,
                              gameStatePrev.players[i].position+PADDLE_LEN_D2,
                              HORIZ_CENTER_MIN_PL+PADDLE_WID_D2,
                              HORIZ_CENTER_MIN_PL-PADDLE_WID_D2,
                              BACK_COLOR);
            LCD_DrawRectangle(gameState.players[i].position-PADDLE_LEN_D2,
                              gameStatePrev.players[i].position-PADDLE_LEN_D2,
                              HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                              HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                              PLAYER_BLUE);
          }
        }else if(posDiff < 0 && posDiff > -PADDLE_LEN_D2){
          if(i == 0){
            LCD_DrawRectangle(gameState.players[i].position-PADDLE_LEN_D2,
                              gameStatePrev.players[i].position-PADDLE_LEN_D2,
                              HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                              HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                              BACK_COLOR);
            LCD_DrawRectangle(gameState.players[i].position+PADDLE_LEN_D2,
                              gameStatePrev.players[i].position+PADDLE_LEN_D2,
                              HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                              HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                              PLAYER_RED);
          }else{
            LCD_DrawRectangle(gameState.players[i].position-PADDLE_LEN_D2,
                              gameStatePrev.players[i].position-PADDLE_LEN_D2,
                              HORIZ_CENTER_MIN_PL+PADDLE_WID_D2,
                              HORIZ_CENTER_MIN_PL-PADDLE_WID_D2,
                              BACK_COLOR);
            LCD_DrawRectangle(gameState.players[i].position+PADDLE_LEN_D2,
                              gameStatePrev.players[i].position+PADDLE_LEN_D2,
                              HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                              HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                              PLAYER_BLUE);
          }
        }else{
          if(i == 0){
            LCD_DrawRectangle(gameStatePrev.players[i].position-PADDLE_LEN_D2,
                              gameStatePrev.players[i].position+PADDLE_LEN_D2,
                              HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                              HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                              BACK_COLOR);
            LCD_DrawRectangle(gameState.players[i].position-PADDLE_LEN_D2,
                              gameState.players[i].position+PADDLE_LEN_D2,
                              HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                              HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                              PLAYER_RED);
          }else{
            LCD_DrawRectangle(gameStatePrev.players[i].position-PADDLE_LEN_D2,
                              gameStatePrev.players[i].position+PADDLE_LEN_D2,
                              HORIZ_CENTER_MIN_PL+PADDLE_WID_D2,
                              HORIZ_CENTER_MIN_PL-PADDLE_WID_D2,
                              BACK_COLOR);
            LCD_DrawRectangle(gameState.players[i].position-PADDLE_LEN_D2,
                              gameState.players[i].position+PADDLE_LEN_D2,
                              HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                              HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                              PLAYER_BLUE);
          }
        }
        gameStatePrev.players[i].position = gameState.players[i].position;
      }
    }
    for(int i = 0; i < MAX_NUM_OF_BALLS; i++){
      if(gameState.balls[i].alive){
        if(gameStatePrev.balls[i].currentCenterX != gameState.balls[i].currentCenterX ||
           gameStatePrev.balls[i].currentCenterY != gameState.balls[i].currentCenterY){
        
          G8RTOS_WaitSemaphore(&lcd);
          LCD_DrawRectangle(gameStatePrev.balls[i].currentCenterX-BALL_SIZE_D2,
                            gameStatePrev.balls[i].currentCenterX+BALL_SIZE_D2,
                            gameStatePrev.balls[i].currentCenterY-BALL_SIZE_D2,
                            gameStatePrev.balls[i].currentCenterY+BALL_SIZE_D2,
                            LCD_BLACK);
          G8RTOS_SignalSemaphore(&lcd);

          gameStatePrev.balls[i].currentCenterY = gameState.balls[i].currentCenterY;
          gameStatePrev.balls[i].currentCenterX = gameState.balls[i].currentCenterX;
          
          G8RTOS_WaitSemaphore(&lcd);
          LCD_DrawRectangle(gameStatePrev.balls[i].currentCenterX-BALL_SIZE_D2,
                  gameStatePrev.balls[i].currentCenterX+BALL_SIZE_D2,
                  gameStatePrev.balls[i].currentCenterY-BALL_SIZE_D2,
                  gameStatePrev.balls[i].currentCenterY+BALL_SIZE_D2,
                  gameStatePrev.balls[i].color);
          G8RTOS_SignalSemaphore(&lcd);
        }
      }else if(gameStatePrev.balls[i].alive != gameState.balls[i].alive){
          G8RTOS_WaitSemaphore(&lcd);
          LCD_DrawRectangle(gameStatePrev.balls[i].currentCenterX-BALL_SIZE_D2,
                            gameStatePrev.balls[i].currentCenterX+BALL_SIZE_D2,
                            gameStatePrev.balls[i].currentCenterY-BALL_SIZE_D2,
                            gameStatePrev.balls[i].currentCenterY+BALL_SIZE_D2,
                            LCD_BLACK);
          G8RTOS_SignalSemaphore(&lcd);
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
