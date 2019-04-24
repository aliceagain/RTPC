/*
 * Game.h
 *
 *  Created on: Feb 27, 2017
 *      Author: danny
 */

#ifndef GAME_H_
#define GAME_H_

/*********************************************** Includes ********************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "G8RTOS/G8RTOS.h"
#include "cc3100_usage.h"
#include <G8RTOS_IPC.h>
#include "LCDLib.h"
/*********************************************** Includes ********************************************************************/

/*********************************************** Externs ********************************************************************/

/* Semaphores here */ 
semaphore_t cc3100;
semaphore_t lcd;
semaphore_t screenSelf;
semaphore_t screenFriend;
semaphore_t globalState;

/*********************************************** Externs ********************************************************************/

/*********************************************** Global Defines ********************************************************************/

/* Size of game arena */
#define DRAWABLE_MIN_X                  33
#define DRAWABLE_MAX_X                  287
#define DRAWABLE_MIN_Y                  20
#define DRAWABLE_MAX_Y                  220

/* Size of objects */
#define BRUSH_MAX                       1
#define BRUSH_MIN                       32

#define MAX_UNDO                        16

/* Background color - Black */
#define BACK_COLOR                   LCD_BLACK

#define SELF 0
#define FRIEND 1

typedef enum {
    point,
    undo
}PacketType;

/*********************************************** Global Defines ********************************************************************/

/*********************************************** Data Structures ********************************************************************/
/*********************************************** Data Structures ********************************************************************/
#pragma pack ( push, 1)
/*
 * Struct to be sent to and from the client and the host
 */
typedef struct{
    PacketType header;
    BrushStroke_t stroke;
} Packet_t;

#pragma pack ( pop )

typedef struct{
    uint8_t r;
    uint8_t g;
    uint8_t b;
}Color_t;

typedef struct{
    uint8_t x;
    uint8_t y;
}ScreenPos_t;

typedef struct{
    Color_t color;
    ScreenPos_t pos;
}Pixel_t;

typedef struct{
    Color_t color;
    uint8_t size;
}Brush_t;

typedef struct{
    Brush_t brush;
    ScreenPos_t pos;
}BrushStroke_t;

typedef struct{
    Pixel_t board[(DRAWABLE_MAX_X-DRAWABLE_MIN_X)*(DRAWABLE_MAX_Y-DRAWABLE_MIN_Y)];
}Board_t;

typedef struct{
    BrushStroke_t strokes[MAX_UNDO];
}StrokeQueue_t;

/*
 * GameState
 */
typedef struct{
    Brush_t currentBrush;
    Board_t selfBoard;
    Board_t friendBoard;
    StrokeQueue_t selfQueue;
    StrokeQueue_t friendQueue;
    uint8_t currentBoard;
    ScreenPos_t touch;
    int8_t lastSentStroke;
    int8_t lastDrawnStroke;
    uint8_t redrawBoard;
} GameState_t;


/*********************************************** Data Structures ********************************************************************/

/*******************************************************  Threads *********************************************************************/
/*
 * Send packets over wifi
 */
void Send();

/*
 * Receive packets over wifi
 */
void Receive();

/*
 * Read touchpad data into lastTouch
 */

void ReadTouch();

void ProcessTouch();

/*
 * Draw pixels/boardstate
 */
void Draw();

/*
 * Connect two boards together over wifi
 */
void InitialConnection();

void IdleThread();

/*******************************************************  Threads *********************************************************************/

/*********************************************** Public Functions *********************************************************************/

void InitHost();

void InitClient();

void CreateThreadsAndStart();

void DrawBoard();

void RedrawStrokes();

void DrawStroke();

void SendStroke();

void ReceiveStroke();

/*********************************************** Public Functions *********************************************************************/


#endif /* GAME_H_ */
