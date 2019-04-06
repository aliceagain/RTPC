#include "Game.h"

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame(){
  initCC3100(Client);
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
  initCC3100(Host);
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

}

/*
 * Thread to draw all the objects in the game
 */
void DrawObjects(){

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
