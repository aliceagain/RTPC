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
semaphore_t globalState;

/*********************************************** Externs ********************************************************************/

/*********************************************** Global Defines ********************************************************************/

/* Size of game arena */
#define DRAWABLE_MIN_X                  033
#define DRAWABLE_MAX_X                  287
#define DRAWABLE_MIN_Y                  020
#define DRAWABLE_MAX_Y                  220

/* Size of objects */
#define BRUSH_MAX                       1
#define BRUSH_MIN                       32

#define MAX_STROKES                     16

/* Background color - Black */
#define BACK_COLOR                   LCD_BLACK

#define SELF 0
#define FRIEND 1

/* Used as status LEDs for Wi-Fi */
#define BLUE_LED BIT2
#define RED_LED BIT0

#define MIN_DIST 16

typedef enum {
    point,
    undo
}PacketType;

/*********************************************** Global Defines ********************************************************************/

/*********************************************** Data Structures ********************************************************************/
/*********************************************** Data Structures ********************************************************************/
#pragma pack ( push, 1)

#pragma pack ( pop )

typedef struct{
    uint8_t c;
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
    BrushStroke_t strokes[MAX_STROKES];
}StrokeQueue_t;

/*
 * Struct to be sent to and from the client and the host
 */
typedef struct{
    PacketType header;
    BrushStroke_t stroke;
} Packet_t;

/*
 * GameState
 */
typedef struct{
    uint32_t ip;
    Brush_t currentBrush;
    StrokeQueue_t selfQueue;
    StrokeQueue_t friendQueue;
    uin16_t stackPos;
    uint16_t friendStackPos;
    uint8_t currentBoard;
    int8_t lastSentStroke;
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
void WaitInit();

void IdleThread();

/*******************************************************  Threads *********************************************************************/

/*********************************************** Public Functions *********************************************************************/

void InitHost();

void InitClient();

void CreateThreadsAndStart();

void DrawBoard();

void RedrawStrokes();

void DrawStroke(BrushStroke_t * stroke);

void SendStroke(BrushStroke_t * stroke);

void ReceiveStroke(BrushStroke_t * stroke);

/*********************************************** Public Functions *********************************************************************/


#endif /* GAME_H_ */
