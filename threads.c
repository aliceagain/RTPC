/*
 * threads.c
 *
 *  Created on: Feb 27, 2019
 *      Author: Alice
 */

#include "threads.h"
#include <stdio.h>

int16_t accelX;
int16_t accelY;
bool touchReceived = false;
bool updateText = false;
struct ball BallThreads[MAX_BALLS];
uint16_t numberOfBalls = 0;
uint16_t colors[10] = {0xFFFF,0x4197,0xF800,0xF81F,0x57E0,0x7FFF,0xFFE0,0xA104,0xF11F,0xFD20};

void accelerometer(void){
    while(1){
        G8RTOS_WaitSemaphore(&i2c);
        bmi160_read_accel_x(&accelX);
        accelX /= 1400;
        bmi160_read_accel_y(&accelY);
        accelY /= -1400;
        G8RTOS_SignalSemaphore(&i2c);
        sleep(100);
    }
}

void lcdTapHandler(void){
    touchReceived = true;
    P4->IFG &= ~BIT0; // must clear IFG flag// reading PxIV will automatically clear IFG
}

bool nearPoint(uint16_t x, uint16_t y, Point b, uint16_t dist){
    int16_t x_diff = x-b.x;
    int16_t y_diff = y-b.y;
    if(x_diff > -dist && x_diff < dist){
        if(y_diff > -dist && y_diff < dist){
            return true;
        }
    }return false;
}

bool killBall(Point p){
    for(int i = 0; i < MAX_BALLS; i++){
        if(BallThreads[i].isAlive){
            if(nearPoint(BallThreads[i].positionX,BallThreads[i].positionY,p,16)){
                G8RTOS_WaitSemaphore(&lcd);
                G8RTOS_KillThread(BallThreads[i].id);
                BallThreads[i].isAlive = false;
                LCD_DrawRectangle(BallThreads[i].positionX, BallThreads[i].positionX+4,
                                  BallThreads[i].positionY, BallThreads[i].positionY+4,
                                  LCD_BLACK);
                numberOfBalls--;
                G8RTOS_SignalSemaphore(&lcd);
                return true;
            }
        }
    }return false;
}
uint8_t ballText[32] = "Number of Balls: 0";
char* ballThreadName = "BallThread";
void tapResponse(void){
    LCD_Text(0, 227, ballText, LCD_WHITE);
    while(1){
        if(touchReceived){
            Point temp = TP_ReadXY();
            if(temp.x < 325 && temp.y < 245){
                if(!killBall(temp)){
                    if(numberOfBalls < MAX_BALLS){
                        numberOfBalls++;
                        writeFIFO(TOUCH_FIFO, temp.x);
                        writeFIFO(TOUCH_FIFO, temp.y);
                        G8RTOS_AddThread(ballThread, BALL_PRIORITY, ballThreadName);
                    }
                }
                G8RTOS_WaitSemaphore(&lcd);
                LCD_Text(0, 227, ballText, LCD_BLACK);
                G8RTOS_SignalSemaphore(&lcd);
                sprintf(ballText, "Number of Balls: %d",numberOfBalls);
                G8RTOS_WaitSemaphore(&lcd);
                LCD_Text(0, 227, ballText, LCD_WHITE);
                G8RTOS_SignalSemaphore(&lcd);
                sleep(100);
            }
            touchReceived = false;
        }
        if(updateText){
            G8RTOS_WaitSemaphore(&lcd);
            LCD_Text(0, 227, ballText, LCD_WHITE);
            G8RTOS_SignalSemaphore(&lcd);
            updateText = false;
        }
        sleep(50);
    }
}

void idleThread(void){
    while(1);
}

uint8_t ballPos[16] = "X: Y:";
void ballThread(void){
    uint16_t self = MAX_BALLS;
    for(int i = 0; i < MAX_BALLS; i++){
        if(!BallThreads[i].isAlive){
            self = i;
            break;
        }
    }
    if(self == MAX_BALLS){
        G8RTOS_KillSelf();
    }
    BallThreads[self].positionX = readFIFO(TOUCH_FIFO);
    BallThreads[self].positionY = readFIFO(TOUCH_FIFO);
    BallThreads[self].id = G8RTOS_GetThreadId();
    BallThreads[self].isAlive = true;
    BallThreads[self].velocityX = accelX;
    BallThreads[self].velocityY = accelY;
    BallThreads[self].scale = (BallThreads[self].id%4)+1;

    BallThreads[self].color = colors[(self<<4 | BallThreads[self].id)%10];
    G8RTOS_WaitSemaphore(&lcd);
    LCD_DrawRectangle(BallThreads[self].positionX, BallThreads[self].positionX+4,
                      BallThreads[self].positionY, BallThreads[self].positionY+4,
                      BallThreads[self].color);
    G8RTOS_SignalSemaphore(&lcd);
    while(1){
        sleep(30);
        G8RTOS_WaitSemaphore(&lcd);
        LCD_DrawRectangle(BallThreads[self].positionX, BallThreads[self].positionX+4,
                          BallThreads[self].positionY, BallThreads[self].positionY+4,
                          LCD_BLACK);
        G8RTOS_SignalSemaphore(&lcd);
        BallThreads[self].velocityX = accelX*BallThreads[self].scale;
        BallThreads[self].velocityY = accelY*BallThreads[self].scale;
        BallThreads[self].velocityX *= 0.9;
        BallThreads[self].velocityY *= 0.9;
        BallThreads[self].positionX += BallThreads[self].velocityX;
        BallThreads[self].positionY += BallThreads[self].velocityY;
        if(BallThreads[self].positionX < 0){
            BallThreads[self].positionX+= 320;
        }else{
            BallThreads[self].positionX %= 320;
        }
        if(BallThreads[self].positionY < 0){
            BallThreads[self].positionY+= 227;
        }else{
            BallThreads[self].positionY %= 227;
        }
        G8RTOS_WaitSemaphore(&lcd);
        LCD_DrawRectangle(BallThreads[self].positionX, BallThreads[self].positionX+4,
                          BallThreads[self].positionY, BallThreads[self].positionY+4,
                          BallThreads[self].color);
        G8RTOS_SignalSemaphore(&lcd);
    }
}

void ButtonHandler(void){
    updateText = true;
    P5->IFG &= ~BIT5; // must clear IFG flag// reading PxIV will automatically clear IFG
}



