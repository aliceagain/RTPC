#include "Game.h"
#include "BSP.h"

/*********************************************** Global Vars *********************************************************************/
GameState_t gameState;
uint16_t ballColors[8] = {0xF81F,0x07E0,0x7FFF,0xFFE0,0xF11F,0xFD20,0xfdba,0xdfe4};

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame(){
  G8RTOS_AddThread(IdleThread, 255, NULL);
  G8RTOS_InitSemaphore(&cc3100, 1);
  G8RTOS_InitSemaphore(&lcd, 1);
  initCC3100(Client);

  G8RTOS_InitSemaphore(&cc3100, 1);

  int16_t xcoord, ycoord; //just used to initialize joystick displacement (y not necessary)
  GetJoystickCoordinates(&xcoord, &ycoord);

  //init specific player info for client
  gameState.player.IP_address = getLocalIP();
  gameState.player.displacement = xcoord;
  gameState.player.playerNumber = CLIENT;  //0 = client, 1 = host
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

  InitBoardState();

  //change priorities later
  G8RTOS_AddThread(ReadJoystickClient, 4, NULL);
  G8RTOS_AddThread(SendDataToHost, 8, NULL);
  G8RTOS_AddThread(ReceiveDataFromHost, 6, NULL);
  G8RTOS_AddThread(DrawObjects, 5, NULL);
  G8RTOS_AddThread(MoveLEDs, 250, NULL);
  
  G8RTOS_KillSelf();
}

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost(){
  while(1){
    G8RTOS_WaitSemaphore(&cc3100);
    while(ReceiveData((uint8_t *)&gameState, sizeof(GameState_t)) <= 0){
      G8RTOS_SignalSemaphore(&cc3100);  
      sleep(1);
      G8RTOS_WaitSemaphore(&cc3100);
    }
    G8RTOS_SignalSemaphore(&cc3100);

    if(gameState.gameDone == true){
      G8RTOS_AddThread(EndOfGameClient, 0, NULL);
    }
    sleep(5);
  }
}

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost(){
  while(1){
    G8RTOS_WaitSemaphore(&cc3100);
    SendData((uint8_t *)&gameState.player, HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t));
    G8RTOS_SignalSemaphore(&cc3100);
    sleep(2);
  }
}

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient(){
  int16_t xcoord, ycoord, xoffset, yoffset; //ycoord not needed
  GetJoystickCoordinates(&xoffset, &yoffset); //this will get the offset ASSUMING joystick is in neutral position

  //G8RTOS_InitFIFO(JOYSTICK_CLIENTFIFO); 
  while(1){
    GetJoystickCoordinates(&xcoord, &ycoord);   //read x coord;
    //writeFIFO(JOYSTICK_CLIENTFIFO,xcoord - xoffset);
    gameState.player.displacement = xcoord;
    sleep(10);
  }
  
  
}

/*
 * End of game for the client
 */
void EndOfGameClient(){

  G8RTOS_WaitSemaphore(&cc3100);
  G8RTOS_WaitSemaphore(&lcd);

  G8RTOS_KillThread(ReadJoystickClient);
  G8RTOS_KillThread(SendDataToHost);
  G8RTOS_KillThread(ReceiveDataFromHost);
  G8RTOS_KillThread(DrawObjects);
  G8RTOS_KillThread(MoveLEDs);
  G8RTOS_KillThread(IdleThread);

  G8RTOS_SignalSemaphore(&lcd);
  G8RTOS_SignalSemaphore(&cc3100);

  if(gameState.winner == HOST){
    LCD_Clear(gameState.players[HOST].color);  
  }
  else{
    LCD_Clear(gameState.players[CLIENT].color); 
  }

  while(ReceiveData((uint8_t *)&gameState, sizeof(GameState_t)) <= 0);  //** CHECK: wait for new game state to see if restart?

  InitBoardState(); //re-init board state

  //re-add threads
  G8RTOS_AddThread(IdleThread, 255, NULL);
  G8RTOS_AddThread(ReadJoystickClient, 4, NULL);
  G8RTOS_AddThread(SendDataToHost, 8, NULL);
  G8RTOS_AddThread(ReceiveDataFromHost, 6, NULL);
  G8RTOS_AddThread(DrawObjects, 5, NULL);
  G8RTOS_AddThread(MoveLEDs, 250, NULL);

  //reset game variables ***

  G8RTOS_KillSelf();



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

  gameState.players[HOST]= (GeneralPlayerInfo_t){PADDLE_X_CENTER, PLAYER_RED, BOTTOM};
  gameState.players[CLIENT]= (GeneralPlayerInfo_t){PADDLE_X_CENTER, PLAYER_BLUE, TOP};

  G8RTOS_WaitSemaphore(&cc3100);
  initCC3100(Host);

  while(ReceiveData((uint8_t * )&gameState.player, sizeof(SpecificPlayerInfo_t)) == NOTHING_RECEIVED);
  gameState.player.joined = true;
  gameState.player.acknowledge = true;
  SendData((uint8_t * ) &gameState.player, gameState.player.IP_address, sizeof(gameState.player));
  G8RTOS_SignalSemaphore(&cc3100);

  P2->DIR |= RED_LED;  //p1.0 set to output
  BITBAND_PERI(P2->OUT, 0) = 1;  //toggle led on

  InitBoardState();

  //change priorities later
  G8RTOS_AddThread(GenerateBall, 8, NULL);
  G8RTOS_AddThread(DrawObjects, 5, NULL);
  G8RTOS_AddThread(ReadJoystickHost, 4, NULL);
  G8RTOS_AddThread(SendDataToClient, 7, NULL);
  G8RTOS_AddThread(ReceiveDataFromClient, 6, NULL);
  G8RTOS_AddThread(MoveLEDs, 250, NULL);

  G8RTOS_KillSelf();

  while(1);
}

/*
 * Thread that sends game state to client
 */
void SendDataToClient(){
  while(1){
    SendData((uint8_t * ) &gameState, gameState.player.IP_address, sizeof(gameState));
    if(gameState.gameDone){
      G8RTOS_AddThread(EndOfGameHost, 0, NULL);
    }
    sleep(5);
  }
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
    gameState.players[CLIENT].currentCenter += tempDisplacement;
    if(gameState.players[CLIENT].currentCenter > HORIZ_CENTER_MAX_PL){
      gameState.players[CLIENT].currentCenter = HORIZ_CENTER_MAX_PL;
    }else if(gameState.players[CLIENT].currentCenter < HORIZ_CENTER_MIN_PL){
      gameState.players[CLIENT].currentCenter = HORIZ_CENTER_MIN_PL;
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
      gameState.numberOfBalls++;
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
    if(xcoord < 1000 && xcoord > -1000 ){
        xcoord = 0;
    }
    xcoord>>=9;
    sleep(10);
    gameState.players[HOST].currentCenter += xcoord;
  }  
}

/*
 * Thread to move a single ball
 */
void MoveBall(){
  Ball_t * ball = NULL;
  for(int i = 0; i < MAX_NUM_OF_BALLS; i++){
      if(!gameState.balls[i].alive){
        ball = &gameState.balls[i];
      }
  }
  if(!ball){
    G8RTOS_KillSelf();
  }
  ball->currentCenterX = (SystemTime-(int32_t)&ball>>2+gameState.players[CLIENT].currentCenter)%HORIZ_CENTER_MAX_BALL + HORIZ_CENTER_MIN_BALL;
  ball->currentCenterY = (SystemTime-(int32_t)&ball>>2+gameState.players[HOST].currentCenter)%VERT_CENTER_MAX_BALL + VERT_CENTER_MIN_BALL;
  ball->currentVelocityX = (SystemTime-(int32_t)&ball>>2+gameState.players[HOST].currentCenter)%MAX_BALL_SPEED;
  ball->currentVelocityY = (SystemTime-(int32_t)&ball>>2+gameState.players[CLIENT].currentCenter)%MAX_BALL_SPEED;
  ball->color = ballColors[SystemTime>>3%8];
  ball->alive = true;
  while(1){
    ball->currentCenterX += ball->currentVelocityX;
    ball->currentCenterY += ball->currentVelocityY;
    if(ball->currentCenterX > HORIZ_CENTER_MAX_BALL){
      ball->currentVelocityX *= -1;
      ball->currentCenterX = HORIZ_CENTER_MAX_BALL;
    }
    if(ball->currentCenterX < HORIZ_CENTER_MIN_BALL){
      ball->currentVelocityX *= -1;
      ball->currentCenterX = HORIZ_CENTER_MIN_BALL;
    }
    if(ball->currentCenterY > VERT_CENTER_MAX_BALL){
      gameState.LEDScores[CLIENT] |= 0x10<<gameState.overallScores[CLIENT]++;
      if(gameState.overallScores[CLIENT] == 4){
        gameState.winner = CLIENT;
        gameState.gameDone = true;
      }
      G8RTOS_WaitSemaphore(&lcd);
      LCD_DrawRectangle(ball->currentCenterX-BALL_SIZE_D2,
                        ball->currentCenterX+BALL_SIZE_D2,
                        ball->currentCenterY-BALL_SIZE_D2,
                        ball->currentCenterY+BALL_SIZE_D2,
                        LCD_BLACK);
      G8RTOS_SignalSemaphore(&lcd);
      G8RTOS_KillSelf();
    }
    if(ball->currentCenterY < VERT_CENTER_MIN_BALL){
      gameState.LEDScores[HOST] |= 0xF>>gameState.overallScores[HOST]++;
      if(gameState.overallScores[HOST] == 4){
        gameState.winner = HOST;
        gameState.gameDone = true;
      }
      G8RTOS_WaitSemaphore(&lcd);
      LCD_DrawRectangle(ball->currentCenterX-BALL_SIZE_D2,
                        ball->currentCenterX+BALL_SIZE_D2,
                        ball->currentCenterY-BALL_SIZE_D2,
                        ball->currentCenterY+BALL_SIZE_D2,
                        LCD_BLACK);
      G8RTOS_SignalSemaphore(&lcd);
      G8RTOS_KillSelf();
    }

    switch (collision((CollsionPos_t){PADDLE_WID,PADDLE_LEN+WIGGLE_ROOM,gameState.players[CLIENT].currentCenter,TOP_PLAYER_CENTER_Y},
                      (CollsionPos_t){BALL_SIZE,BALL_SIZE,ball->currentCenterX,ball->currentCenterY})){
      case CTOP:
      case CBOTTOM:
        ball->currentVelocityY *= -1;
        break;
      case CRIGHT:
      case CLEFT:
        ball->currentVelocityX *= -1;
        break;
      case CNONE:
        break;
    }

    switch (collision((CollsionPos_t){PADDLE_WID,PADDLE_LEN+WIGGLE_ROOM,gameState.players[CLIENT].currentCenter,BOTTOM_PLAYER_CENTER_Y},
                      (CollsionPos_t){BALL_SIZE,BALL_SIZE,ball->currentCenterX,ball->currentCenterY})){
      case CTOP:
      case CBOTTOM:
        ball->currentVelocityY *= -1;
        break;
      case CRIGHT:
      case CLEFT:
        ball->currentVelocityX *= -1;
        break;
      case CNONE:
        break;
    }

    sleep(35);
  }
}

bool restart = false;
void EndofGameButtonHandler(){
  restart = true;
  P4->IFG &= ~BIT4;  
}

/*
 * End of game for the host
 */
char endOfGameText[] = "Press Host Button on Host to Restart Game.";
void EndOfGameHost(){
    G8RTOS_WaitSemaphore(&cc3100);
    G8RTOS_WaitSemaphore(&lcd);
    G8RTOS_KillOthers();
    G8RTOS_InitSemaphore(&cc3100, 1);
    G8RTOS_InitSemaphore(&lcd, 1);
    
    G8RTOS_WaitSemaphore(&lcd);
    LCD_Clear(gameState.winner == HOST ? gameState.players[HOST].color : gameState.players[CLIENT].color);
    LCD_Text(20,100,(uint8_t*)endOfGameText,LCD_BLACK);
    P4->IE |= BIT4;
    G8RTOS_AddAPeriodicEvent(EndofGameButtonHandler,6,PORT4_IRQn);
    while(!restart);
    restart = false;

    InitBoardState();
    SendData((uint8_t * ) &gameState, gameState.player.IP_address, sizeof(gameState));


    //change priorities later
    G8RTOS_AddThread(GenerateBall, 8, NULL);
    G8RTOS_AddThread(DrawObjects, 5, NULL);
    G8RTOS_AddThread(ReadJoystickHost, 4, NULL);
    G8RTOS_AddThread(SendDataToClient, 7, NULL);
    G8RTOS_AddThread(ReceiveDataFromClient, 6, NULL);
    G8RTOS_AddThread(MoveLEDs, 250, NULL);
    G8RTOS_AddThread(IdleThread, 255, NULL);

    G8RTOS_KillSelf();
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
  PrevPlayer_t prevPlayers[MAX_NUM_OF_PLAYERS];
  PrevBall_t prevBalls[MAX_NUM_OF_BALLS];
  while(1){
    for(int i = 0; i < MAX_NUM_OF_PLAYERS; i++){
      if(prevPlayers[i].Center != gameState.players[i].currentCenter){
        UpdatePlayerOnScreen(&prevPlayers[i],&gameState.players[i]);
      }
    }
    for(int i = 0; i < MAX_NUM_OF_BALLS; i++){
      UpdateBallOnScreen(&prevBalls[i],&gameState.balls[i],gameState.balls[i].color);
    }
    sleep(20);
  }
}

/*
 * Thread to update LEDs based on score
 */
void MoveLEDs(){
  while(1){
    LP3943_LedModeSet(RED,gameState.LEDScores[HOST]);
    LP3943_LedModeSet(BLUE,gameState.LEDScores[CLIENT]);
    sleep(10);
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
        G8RTOS_AddThread(CreateGame,1,NULL);
    }else if(isClient){
        G8RTOS_AddThread(JoinGame,1,NULL);
    }
    G8RTOS_KillSelf();
}

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t * player){
  LCD_DrawRectangle(player->position-PADDLE_LEN_D2,
                  player->position+PADDLE_LEN_D2,
                  HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                  HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                  player->color);
}

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer){
  int16_t posDiff = outPlayer->currentCenter - prevPlayerIn->Center;
  if(posDiff > 0 && posDiff < PADDLE_LEN_D2 ){
    LCD_DrawRectangle(outPlayer->currentCenter+PADDLE_LEN_D2,
                      prevPlayerIn->Center+PADDLE_LEN_D2,
                      HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                      HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                      BACK_COLOR);
    LCD_DrawRectangle(outPlayer->currentCenter-PADDLE_LEN_D2,
                      prevPlayerIn->Center-PADDLE_LEN_D2,
                      HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                      HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                      outPlayer->color);
  }else if(posDiff < 0 && posDiff > -PADDLE_LEN_D2){
    LCD_DrawRectangle(outPlayer->currentCenter-PADDLE_LEN_D2,
                      prevPlayerIn->Center-PADDLE_LEN_D2,
                      HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                      HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                      BACK_COLOR);
    LCD_DrawRectangle(outPlayer->currentCenter+PADDLE_LEN_D2,
                      prevPlayerIn->Center+PADDLE_LEN_D2,
                      HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                      HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                      outPlayer->color);
  }else{
    LCD_DrawRectangle(prevPlayerIn->Center-PADDLE_LEN_D2,
                      prevPlayerIn->Center+PADDLE_LEN_D2,
                      HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                      HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                      BACK_COLOR);
    LCD_DrawRectangle(outPlayer->currentCenter-PADDLE_LEN_D2,
                      outPlayer->currentCenter+PADDLE_LEN_D2,
                      HORIZ_CENTER_MAX_PL+PADDLE_WID_D2,
                      HORIZ_CENTER_MAX_PL-PADDLE_WID_D2,
                      outPlayer->color);
  }
  prevPlayerIn->Center = outPlayer->currentCenter;
}

/*
 * Function updates ball position on screen
 */
void UpdateBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall, uint16_t outColor){
  if(currentBall->alive){
    if(previousBall->CenterX != currentBall->currentCenterX ||
        previousBall->CenterY != currentBall->currentCenterY){
    
      G8RTOS_WaitSemaphore(&lcd);
      LCD_DrawRectangle(previousBall->CenterX-BALL_SIZE_D2,
                        previousBall->CenterX+BALL_SIZE_D2,
                        previousBall->CenterY-BALL_SIZE_D2,
                        previousBall->CenterY+BALL_SIZE_D2,
                        LCD_BLACK);
      G8RTOS_SignalSemaphore(&lcd);

      previousBall->CenterY = currentBall->currentCenterY;
      previousBall->CenterX = currentBall->currentCenterX;
      
      G8RTOS_WaitSemaphore(&lcd);
      LCD_DrawRectangle(previousBall->CenterX-BALL_SIZE_D2,
              previousBall->CenterX+BALL_SIZE_D2,
              previousBall->CenterY-BALL_SIZE_D2,
              previousBall->CenterY+BALL_SIZE_D2,
              outColor);
      G8RTOS_SignalSemaphore(&lcd);
    }
  }
}

/*
 * Initializes and prints initial game state
 */
void InitBoardState(){
  gameState.players[HOST].currentCenter = PADDLE_X_CENTER;
  gameState.players[CLIENT].currentCenter = PADDLE_X_CENTER;
  gameState.winner = 0;
  gameState.gameDone = false;
  for(int i = 0; i < MAX_NUM_OF_BALLS; i++){
    gameState.balls[i].alive = false;
  }
  gameState.numberOfBalls = 0;
  for(int i = 0; i < MAX_NUM_OF_PLAYERS; i++){
    gameState.LEDScores[i] = 0;
    gameState.overallScores[i] = 0;
  }

  G8RTOS_WaitSemaphore(&lcd);
  LCD_Init(false);
  LCD_Clear(LCD_GRAY);
  LCD_DrawRectangle(ARENA_MIN_X, ARENA_MAX_X, ARENA_MIN_X, ARENA_MAX_Y,LCD_BLACK);
  G8RTOS_SignalSemaphore(&lcd);

  DrawPlayer(&gameState.players[HOST]);
  DrawPlayer(&gameState.players[CLIENT]);
}

collisionPosition collision(CollsionPos_t A, CollsionPos_t B){
  int32_t w = 0.5 * (A.width + B.width);
  int32_t h = 0.5 * (A.height + B.height);
  int32_t dx = A.centerX - B.centerX;
  int32_t dy = A.centerY - B.centerY;

  if (abs(dx) <= w && abs(dy) <= h){
    /* collision! */
    int32_t wy = w * dy;
    int32_t hx = h * dx;
    if (wy > hx){
      if (wy > -hx){
        return CTOP;
      }else{
        return CBOTTOM;
      }
    }else{
      if (wy > -hx){
       return CRIGHT;
      }else{
        return CLEFT;
      }
    }
  }
  return CNONE;
}

/*********************************************** Public Functions *********************************************************************/
