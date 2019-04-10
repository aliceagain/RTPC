#include "Game.h"
#include "BSP.h"

/*********************************************** Global Vars *********************************************************************/
GameState_t gameState;
char readjoystickName[] = "readJoystick";
char idlethreadName[] = "idle";
char senddatatName[] = "sendData";
char receivedataName[] = "receiveData";
char drawobjectsName[] = "DrawObjects";
char moveledsName[] = "MoveLEDs";
char endofgameName[] = "EndofGame";
char generateballName[] = "GenerateBall";
char moveballName[] = "MoveBall";

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
  gameState.player.displacement = 0;
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

  G8RTOS_AddThread(MoveLEDs, 250, moveledsName);
  G8RTOS_AddThread(DrawObjects, 1, drawobjectsName);
  G8RTOS_AddThread(ReadJoystickClient, 4, readjoystickName);
  G8RTOS_AddThread(SendDataToHost, 3, senddatatName);
  G8RTOS_AddThread(ReceiveDataFromHost, 2, receivedataName);

  G8RTOS_AddThread(IdleThread, 254, idlethreadName);
  
  G8RTOS_KillSelf();
}

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost(){
  int16_t inpacket[40];
  while(1){
    G8RTOS_WaitSemaphore(&cc3100);
    while(ReceiveData((uint8_t *)&inpacket, sizeof(int16_t) * 40) == NOTHING_RECEIVED){
      G8RTOS_SignalSemaphore(&cc3100);  

      sleep(1);
      G8RTOS_WaitSemaphore(&cc3100);
    }
    G8RTOS_SignalSemaphore(&cc3100);

    G8RTOS_WaitSemaphore(&player);
    gameState.players[0].currentCenter = inpacket[0];
    gameState.players[1].currentCenter = inpacket[1];
    for(int i = 0; i < MAX_NUM_OF_BALLS; i++){
      gameState.balls[i].currentCenterX = inpacket[2+i*4];
      gameState.balls[i].currentCenterY = inpacket[3+i*4];
      gameState.balls[i].color = inpacket[4+i*4];
      gameState.balls[i].alive = inpacket[5+i*4];
    }
    gameState.winner = inpacket[34];
    gameState.gameDone = inpacket[35];
    gameState.LEDScores[0] = inpacket[36];
    gameState.LEDScores[1] = inpacket[37];
    gameState.overallScores[0] = inpacket[38];
    gameState.overallScores[1] = inpacket[39];
    G8RTOS_SignalSemaphore(&player);


    if(gameState.gameDone == true){
      G8RTOS_AddThread(EndOfGameClient, 0, endofgameName);
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
    SendData((uint8_t *)&gameState.player.displacement, HOST_IP_ADDR, sizeof(int16_t));
    G8RTOS_SignalSemaphore(&cc3100);
    sleep(2);
  }
}

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient(){
  int16_t xcoord, ycoord;

  while(1){
    GetJoystickCoordinates(&xcoord, &ycoord);   //read x coord;
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
  G8RTOS_WaitSemaphore(&player);

  G8RTOS_KillOthers();

  G8RTOS_InitSemaphore(&lcd, 1);
  G8RTOS_InitSemaphore(&cc3100, 1);
  G8RTOS_InitSemaphore(&player, 1);

  G8RTOS_WaitSemaphore(&lcd);
  if(gameState.winner == HOST){
    LCD_Clear(gameState.players[HOST].color);  
  }
  else{
    LCD_Clear(gameState.players[CLIENT].color); 
  }
  G8RTOS_SignalSemaphore(&lcd);

  G8RTOS_WaitSemaphore(&cc3100);
  while(ReceiveData((uint8_t *)&gameState, sizeof(GameState_t)) <= 0);  //** CHECK: wait for new game state to see if restart?
  G8RTOS_SignalSemaphore(&cc3100);

  InitBoardState(); //re-init board state

  //re-add threads
  G8RTOS_AddThread(MoveLEDs, 250, moveledsName);
  G8RTOS_AddThread(DrawObjects, 5, drawobjectsName);
  G8RTOS_AddThread(ReadJoystickClient, 4, readjoystickName);
  G8RTOS_AddThread(SendDataToHost, 3, senddatatName);
  G8RTOS_AddThread(ReceiveDataFromHost, 2, receivedataName);
  G8RTOS_AddThread(IdleThread, 254, idlethreadName);

  //reset game variables ***

  G8RTOS_KillSelf();



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

  gameState.players[HOST]= (GeneralPlayerInfo_t){PADDLE_X_CENTER, PLAYER_RED, BOTTOM};
  gameState.players[CLIENT]= (GeneralPlayerInfo_t){PADDLE_X_CENTER, PLAYER_BLUE, TOP};

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
  G8RTOS_AddThread(MoveLEDs, 250, moveledsName);
  G8RTOS_AddThread(GenerateBall, 8, generateballName);
  G8RTOS_AddThread(DrawObjects, 1, drawobjectsName);
  G8RTOS_AddThread(ReadJoystickHost, 4, readjoystickName);
  G8RTOS_AddThread(SendDataToClient, 2, senddatatName);
  G8RTOS_AddThread(ReceiveDataFromClient, 3, receivedataName);
  G8RTOS_AddThread(IdleThread,254,idlethreadName);


  G8RTOS_KillSelf();

  while(1);
}

/*
 * Thread that sends game state to client
 */
void SendDataToClient(){
  int16_t outPacket[40];
  while(1){
    G8RTOS_WaitSemaphore(&player);
    outPacket[0] = gameState.players[0].currentCenter;
    outPacket[1] = gameState.players[1].currentCenter;
    for(int i = 0; i < MAX_NUM_OF_BALLS; i++){
      outPacket[2+i*4] = gameState.balls[i].currentCenterX;
      outPacket[3+i*4] = gameState.balls[i].currentCenterY;
      outPacket[4+i*4] = gameState.balls[i].color;
      outPacket[5+i*4] = gameState.balls[i].alive;
    }
    outPacket[34] = gameState.winner;
    outPacket[35] = gameState.gameDone;
    outPacket[36] = gameState.LEDScores[0];
    outPacket[37] = gameState.LEDScores[1];
    outPacket[38] = gameState.overallScores[0];
    outPacket[39] = gameState.overallScores[1];
    G8RTOS_SignalSemaphore(&player);
    G8RTOS_WaitSemaphore(&cc3100);
    SendData((uint8_t * ) outPacket, gameState.player.IP_address, sizeof(int16_t)*40);
    G8RTOS_SignalSemaphore(&cc3100);
    if(gameState.gameDone){
      G8RTOS_AddThread(EndOfGameHost, 0, endofgameName);
    }
    sleep(5);
  }
}

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient(){
  int16_t tempDisplacement;
  while(1){
   G8RTOS_WaitSemaphore(&cc3100);
   while(ReceiveData((uint8_t * )&tempDisplacement, sizeof(int16_t)) == NOTHING_RECEIVED){
     G8RTOS_SignalSemaphore(&cc3100);
     sleep(1);
     G8RTOS_WaitSemaphore(&cc3100);
   }
   G8RTOS_SignalSemaphore(&cc3100);
   tempDisplacement += 500;
   if(tempDisplacement < 1000 && tempDisplacement > -1000 ){
       tempDisplacement = 0;
   }
   tempDisplacement>>=11;
   G8RTOS_WaitSemaphore(&player);
   gameState.players[CLIENT].currentCenter += -tempDisplacement;
   if(gameState.players[CLIENT].currentCenter > HORIZ_CENTER_MAX_PL){
     gameState.players[CLIENT].currentCenter = HORIZ_CENTER_MAX_PL;
   }else if(gameState.players[CLIENT].currentCenter < HORIZ_CENTER_MIN_PL){
     gameState.players[CLIENT].currentCenter = HORIZ_CENTER_MIN_PL;
   }
   G8RTOS_SignalSemaphore(&player);
    sleep(2);
  }
}

/*
 * Generate Ball thread
 */
void GenerateBall(){
  while(1){
    if(gameState.numberOfBalls < MAX_NUM_OF_BALLS){
      G8RTOS_AddThread(MoveBall,100,moveballName);
      gameState.numberOfBalls++;
    }
    sleep(10000*gameState.numberOfBalls);
  }
}

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost(){
  int16_t xcoord, ycoord; //ycoord not needed
  while(1){
    GetJoystickCoordinates(&xcoord, &ycoord);   //read x coord;
    xcoord += 500;
    if(xcoord < 1000 && xcoord > -1000 ){
        xcoord = 0;
    }
    xcoord>>=10;
    sleep(10);
    G8RTOS_WaitSemaphore(&player);
    gameState.players[HOST].currentCenter += -xcoord;
    if(gameState.players[HOST].currentCenter > HORIZ_CENTER_MAX_PL){
      gameState.players[HOST].currentCenter = HORIZ_CENTER_MAX_PL;
    }else if(gameState.players[HOST].currentCenter < HORIZ_CENTER_MIN_PL){
      gameState.players[HOST].currentCenter = HORIZ_CENTER_MIN_PL;
    }
    G8RTOS_SignalSemaphore(&player);
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
        break;
      }
  }
  if(!ball){
    G8RTOS_KillSelf();
  }
  ball->currentCenterX = (gameState.players[CLIENT].currentCenter)%HORIZ_CENTER_MAX_BALL + HORIZ_CENTER_MIN_BALL;
  ball->currentCenterY = (gameState.players[HOST].currentCenter)%VERT_CENTER_MAX_BALL + VERT_CENTER_MIN_BALL;
  ball->currentVelocityX = (gameState.players[HOST].currentCenter)%MAX_BALL_SPEED;
  ball->currentVelocityY = (gameState.players[CLIENT].currentCenter)%MAX_BALL_SPEED;
  ball->color = LCD_WHITE;
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
        ball->color = gameState.players[CLIENT].color;
        break;
      case CRIGHT:
      case CLEFT:
        ball->currentVelocityY *= -1;
        ball->color = gameState.players[CLIENT].color;
        break;
      case CNONE:
        break;
    }

    switch (collision((CollsionPos_t){PADDLE_WID,PADDLE_LEN+WIGGLE_ROOM,gameState.players[CLIENT].currentCenter,BOTTOM_PLAYER_CENTER_Y},
                      (CollsionPos_t){BALL_SIZE,BALL_SIZE,ball->currentCenterX,ball->currentCenterY})){
      case CTOP:
      case CBOTTOM:
        ball->currentVelocityY *= -1;
        ball->color = gameState.players[HOST].color;
        break;
      case CRIGHT:
      case CLEFT:
        ball->currentVelocityY *= -1;
        ball->color = gameState.players[HOST].color;
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
    G8RTOS_WaitSemaphore(&player);
    G8RTOS_KillOthers();
    G8RTOS_InitSemaphore(&cc3100, 1);
    G8RTOS_InitSemaphore(&lcd, 1);
    G8RTOS_InitSemaphore(&player, 1);
    
    G8RTOS_WaitSemaphore(&lcd);
    LCD_Clear(gameState.winner == HOST ? gameState.players[HOST].color : gameState.players[CLIENT].color);
    LCD_Text(20,100,(uint8_t*)endOfGameText,LCD_BLACK);
    G8RTOS_SignalSemaphore(&lcd);
    P4->IE |= BIT4;
    G8RTOS_AddAPeriodicEvent(EndofGameButtonHandler,6,PORT4_IRQn);
    while(!restart);
    restart = false;

    InitBoardState();
    G8RTOS_WaitSemaphore(&cc3100);
    SendData((uint8_t * ) &gameState, gameState.player.IP_address, sizeof(gameState));
    G8RTOS_SignalSemaphore(&cc3100);

    //change priorities later
    G8RTOS_AddThread(GenerateBall, 8, generateballName);
    G8RTOS_AddThread(DrawObjects, 5, drawobjectsName);
    G8RTOS_AddThread(ReadJoystickHost, 4, readjoystickName);
    G8RTOS_AddThread(SendDataToClient, 2, senddatatName);
    G8RTOS_AddThread(ReceiveDataFromClient, 3, receivedataName);
    G8RTOS_AddThread(MoveLEDs, 250, moveledsName);
    G8RTOS_AddThread(IdleThread, 254, idlethreadName);

    G8RTOS_KillSelf();
}

/*********************************************** Host Threads *********************************************************************/


/*********************************************** Common Threads *********************************************************************/
/*
 * Idle thread
 */
void IdleThread(){
  while(1){
      G8RTOS_Yield();
  }
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
        G8RTOS_WaitSemaphore(&player);
        UpdatePlayerOnScreen(&prevPlayers[i],&gameState.players[i]);
        G8RTOS_SignalSemaphore(&player);
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
  G8RTOS_WaitSemaphore(&lcd);
  if(player->position == TOP){
      LCD_DrawRectangle(player->currentCenter-PADDLE_LEN_D2,
                      player->currentCenter+PADDLE_LEN_D2,
                      TOP_PLAYER_CENTER_Y-PADDLE_WID_D2,
                      TOP_PLAYER_CENTER_Y+PADDLE_WID_D2,
                      player->color);
  }else{
      LCD_DrawRectangle(player->currentCenter-PADDLE_LEN_D2,
                      player->currentCenter+PADDLE_LEN_D2,
                      BOTTOM_PLAYER_CENTER_Y-PADDLE_WID_D2,
                      BOTTOM_PLAYER_CENTER_Y+PADDLE_WID_D2,
                      player->color);
  }
  G8RTOS_SignalSemaphore(&lcd);
}

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer){
  int16_t posDiff = outPlayer->currentCenter - prevPlayerIn->Center;
  G8RTOS_WaitSemaphore(&lcd);
  int16_t startY;
  int16_t stopY;
  if(outPlayer->position == TOP){
      startY = TOP_PLAYER_CENTER_Y-PADDLE_WID_D2;
      stopY = TOP_PLAYER_CENTER_Y+PADDLE_WID_D2;
  }else{
      startY = BOTTOM_PLAYER_CENTER_Y-PADDLE_WID_D2;
      stopY = BOTTOM_PLAYER_CENTER_Y+PADDLE_WID_D2;
  }
  if(posDiff > 0 && posDiff < PADDLE_LEN_D2 ){
      LCD_DrawRectangle(prevPlayerIn->Center-PADDLE_LEN_D2,
                        outPlayer->currentCenter-PADDLE_LEN_D2,
                        startY,
                        stopY,
                        BACK_COLOR);
      LCD_DrawRectangle(prevPlayerIn->Center+PADDLE_LEN_D2,
                        outPlayer->currentCenter+PADDLE_LEN_D2,
                        startY,
                        stopY,
                        outPlayer->color);
  }else if(posDiff < 0 && posDiff > -PADDLE_LEN_D2){
    LCD_DrawRectangle(outPlayer->currentCenter+PADDLE_LEN_D2,
                      prevPlayerIn->Center+PADDLE_LEN_D2,
                      startY,
                      stopY,
                      BACK_COLOR);
    LCD_DrawRectangle(outPlayer->currentCenter-PADDLE_LEN_D2,
                      prevPlayerIn->Center-PADDLE_LEN_D2,
                      startY,
                      stopY,
                      outPlayer->color);
  }else{
    LCD_DrawRectangle(prevPlayerIn->Center-PADDLE_LEN_D2,
                      prevPlayerIn->Center+PADDLE_LEN_D2,
                      startY,
                      stopY,
                      BACK_COLOR);
    LCD_DrawRectangle(outPlayer->currentCenter-PADDLE_LEN_D2,
                      outPlayer->currentCenter+PADDLE_LEN_D2,
                      startY,
                      stopY,
                      outPlayer->color);
  }
  G8RTOS_SignalSemaphore(&lcd);
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
  LCD_DrawRectangle(ARENA_MIN_X, ARENA_MAX_X, ARENA_MIN_Y, ARENA_MAX_Y,LCD_BLACK);
  G8RTOS_SignalSemaphore(&lcd);

  DrawPlayer(&gameState.players[HOST]);
  DrawPlayer(&gameState.players[CLIENT]);
}

collisionPosition collision(CollsionPos_t A, CollsionPos_t B){
  int32_t h = (A.width + B.width)>>1;
  int32_t w = (A.height + B.height)>>1;
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
