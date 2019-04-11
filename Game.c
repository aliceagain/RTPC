#include "Game.h"
#include "BSP.h"
#include "stdio.h"
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
char scoreClient[] = "0";
char scoreHost[] = "0";

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

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient(){
  int16_t xcoord, ycoord;

  while(1){
    GetJoystickCoordinates(&xcoord, &ycoord);   //read x coord;
    gameState.player.displacementX = xcoord;
    gameState.player.displacementY = ycoord;
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
  G8RTOS_InitFIFO(HOST);
  G8RTOS_InitFIFO(CLIENT);
  gameState.players[HOST]= (GeneralPlayerInfo_t){100, 100, 0, 0, LCD_RED, 1, 0};
  gameState.players[CLIENT]= (GeneralPlayerInfo_t){200, 200, 0, 0, LCD_BLUE, 1, 0};
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

/*
 * Thread that sends game state to client
 */
void SendDataToClient(){
  int16_t outPacket[12];
  while(1){
    G8RTOS_WaitSemaphore(&player);
    outPacket[0] = gameState.players[0].headX;
    outPacket[1] = gameState.players[0].headY;
    outPacket[2] = gameState.players[0].tailX;
    outPacket[3] = gameState.players[0].tailY;
    outPacket[4] = gameState.players[1].headX;
    outPacket[5] = gameState.players[1].headY;
    outPacket[6] = gameState.players[1].tailX;
    outPacket[7] = gameState.players[1].tailY;
    outPacket[8] = gameState.snack.currentCenterX;
    outPacket[9] = gameState.snack.currentCenterY;
    outPacket[10] = gameState.winner;
    outPacket[11] = gameState.gameDone;
    G8RTOS_SignalSemaphore(&player);
    G8RTOS_WaitSemaphore(&cc3100);
    SendData((uint8_t * ) outPacket, gameState.player.IP_address, sizeof(int16_t)*12);
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
  int16_t tempDisplacement[2];
  int16_t velocityX = 1;
  int16_t velocityY = 0;
  while(1){
   G8RTOS_WaitSemaphore(&cc3100);
   while(ReceiveData((uint8_t * )&tempDisplacement, sizeof(int16_t)*2) == NOTHING_RECEIVED){
     G8RTOS_SignalSemaphore(&cc3100);
     sleep(1);
     G8RTOS_WaitSemaphore(&cc3100);
   }
   G8RTOS_SignalSemaphore(&cc3100);
   if(!((tempDisplacement[0] > 3000 || tempDisplacement[0] < -3000)&&(tempDisplacement[1] > 3000 || tempDisplacement[1] < -3000))){
      if(velocityY != 0){
        if(tempDisplacement[0] > 3000){
          velocityX = -4;
          velocityY = 0;
        }else if(tempDisplacement[0] < -3000){
          velocityX = 4;
          velocityY = 0;
        }
      }else if(tempDisplacement[0] != 0){
        if(tempDisplacement[1] > 3000){
          velocityY = 4;
          velocityX = 0;
        }else if(tempDisplacement[1] < -3000){
          velocityY = -4;
          velocityX = 0;
        }
      }
   }

   G8RTOS_WaitSemaphore(&player);
   gameState.players[CLIENT].headX += velocityX;
   gameState.players[CLIENT].headY += velocityY;
   if(gameState.players[CLIENT].headX < 0){
     gameState.players[CLIENT].headX += 320;
   }else{
     gameState.players[CLIENT].headX %= 320;
   }
   if(gameState.players[CLIENT].headY < 0){
     gameState.players[CLIENT].headY += 240;
   }else{
     gameState.players[CLIENT].headY %= 240;
   }
   G8RTOS_SignalSemaphore(&player);
   writeFIFO(CLIENT,gameState.players[CLIENT].headX);
   writeFIFO(CLIENT,gameState.players[CLIENT].headY);
   sleep(2);
  }
}

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost(){
  int16_t xcoord, ycoord; //ycoord not needed
  int16_t velocityX = 0;
  int16_t velocityY = 1;
  while(1){
    GetJoystickCoordinates(&xcoord, &ycoord);   //read x coord;
    if(!((xcoord > 3000 || xcoord < -3000)&&(ycoord > 3000 || ycoord < -3000))){
        if(velocityY != 0){
            if(xcoord > 3000){
                velocityX = -4;
                velocityY = 0;
            }else if(xcoord < -3000){
                velocityX = 4;
                velocityY = 0;
            }
        }else if(velocityX != 0){
            if(ycoord > 3000){
                velocityY = 4;
                velocityX = 0;
            }else if(ycoord < -3000){
                velocityY = -4;
                velocityX = 0;
            }
        }
    }
    sleep(10);
    G8RTOS_WaitSemaphore(&player);
    gameState.players[HOST].headX += velocityX;
    gameState.players[HOST].headY += velocityY;
    if(gameState.players[HOST].headX < 0){
      gameState.players[HOST].headX += 320;
    }else{
      gameState.players[HOST].headX %= 320;
    }
    if(gameState.players[HOST].headY < 0){
      gameState.players[HOST].headY += 240;
    }else{
      gameState.players[HOST].headY %= 240;
    }
    G8RTOS_SignalSemaphore(&player);
    writeFIFO(HOST,gameState.players[HOST].headX);
    writeFIFO(HOST,gameState.players[HOST].headY);
  }  
}

/*
 * Thread to move a single ball
 */

bool restart = false;
void EndofGameButtonHandler(){
  restart = true;
  P4->IFG &= ~BIT4;
  P4->IE &= ~BIT4;
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
  while(1){
    for(int i = 0; i < MAX_NUM_OF_PLAYERS; i++){
      if(prevPlayers[i].headX != gameState.players[i].headX || prevPlayers[i].headY != gameState.players[i].headY){  //if player has moved
        G8RTOS_WaitSemaphore(&player);
        int16_t tempX = readFIFO(i);
        int16_t tempY = readFIFO(i);
        gameState.players[i].tailX = tempX;
        gameState.players[i].tailY = tempY;
        UpdatePlayerOnScreen(&prevPlayers[i],&gameState.players[i]);
        G8RTOS_SignalSemaphore(&player);
      }
    }
    
    sleep(20);
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
      LCD_DrawRectangle(player->headX,
                      player->headX + BALL_SIZE,
                      player->headY,
                      player->headY + BALL_SIZE,
                      player->color);
  
  G8RTOS_SignalSemaphore(&lcd);
}

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prev, GeneralPlayerInfo_t * player){
  G8RTOS_WaitSemaphore(&lcd);
  if(player->currentSize < player->size){
      player->currentSize++;
  }else{
      LCD_DrawRectangle(player->tailX,
                        player->tailX + BALL_SIZE,
                        player->tailY,
                        player->tailY + BALL_SIZE,
                        LCD_BLACK);
  }
  LCD_DrawRectangle(player->headX,
                    player->headX + BALL_SIZE,
                    player->headY,
                    player->headY + BALL_SIZE,
                    player->color);
  G8RTOS_SignalSemaphore(&lcd);
  prev->headX = player->headX;
  prev->headY = player->headY;
}
/*
 * Initializes and prints initial game state
 */
void InitBoardState(){
  G8RTOS_WaitSemaphore(&lcd);
  LCD_Init(false);
  G8RTOS_SignalSemaphore(&lcd);

  DrawPlayer(&gameState.players[HOST]);
  DrawPlayer(&gameState.players[CLIENT]);
}

inline int32_t abs(int32_t x){
  return x < 0 ? -x : x;
}

collisionPosition collision(CollsionPos_t A, CollsionPos_t B){
  int32_t w = (A.width + B.width)>>1;
  int32_t h = (A.height + B.height)>>1;
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
