/*
 * RGBLeds.c
 *
 *  Created on: Jan 17, 2019
 *      Author: Alice
 */
#include "RGBLeds.h"
#include "msp.h"
#include <driverlib.h>

void init_RGBLEDS(){
    uint16_t UNIT_OFF = 0x0000;

    // Software reset enable
    EUSCI_B2->CTLW0 = EUSCI_B_CTLW0_SWRST;

    // Initialize I2C master
    // Set as master, I2C mode, Clock sync, SMCLK source, Transmitter
    EUSCI_B2->CTLW0 |= UCMST     |
                       UCMODE_3  |
                       UCSSEL_2  |
                       UCTR;

    // Set the Fclk as 400khz.
    // Presumes that the SMCLK is selected as source and Fmclk is 12MHz..
    EUSCI_B2->BRW = MAP_CS_getSMCLK()/400000;
    // In conjuction with the next line, this sets the pins as I2C mode.
    // (Table found on p160 of SLAS826E)
    // Set P3.6 as UCB2_SDA and 3.7 as UCB2_SLC
    P3->SEL0 |=  (BIT6 | BIT7);
    P3->SEL1 &= ~(BIT6 | BIT7);

    // Bit wise anding of all bits except UCSWRST.
    EUSCI_B2->CTLW0 &= ~(EUSCI_B_CTLW0_SWRST);

    LP3943_LedModeSet(RED, UNIT_OFF);
    LP3943_LedModeSet(GREEN, UNIT_OFF);
    LP3943_LedModeSet(BLUE, UNIT_OFF);
}

void LP3943_LedModeSet(uint32_t unit, uint16_t LED_DATA){
    /*
     * LP3943_LedModeSet
     * This function will set each of the LEDs to the desired operating
     * mode. The operating modes are on, off, PWM1 and PWM2.
     *
     * The units that can be written to are:
     *      UNIT  |  0  |  Red
     *      UNIT  |  1  |  Blue
     *      UNIT  |  2  |  Green
     *
     * The registers to be written to are:
     *  --------
     *  | LS0  | LED0-3   Selector      |
     *  | LS1  | LED4-7   Selector      |
     *  | LS2  | LED8-11  Selector      |
     *  | LS3  | LED12-16 Selector      |
     *  --------
     */
    uint32_t expandedLED_DATA = 0;
    int i;
    for(i=0; i < 16; ++i){
        expandedLED_DATA <<= 2;
        expandedLED_DATA += ((LED_DATA<<i) & 0x8000)>>15;
    }
    EUSCI_B2->I2CSA  =  0b1100000 + unit;
    EUSCI_B2->CTLW0 |= UCTXSTT; //Start
    while(EUSCI_B2->CTLW0 & UCTXSTT);
    while(!(EUSCI_B2->IFG & UCTXIFG0));
    EUSCI_B2->TXBUF = 0x16;//(autoIncrement | LS0);

    while(!(EUSCI_B2->IFG & UCTXIFG0));
    for(i = 0; i < 4; ++i){
        uint8_t out = (expandedLED_DATA >>(i*8)) & 0x00ff;
        EUSCI_B2->TXBUF = out;
        while(!(EUSCI_B2->IFG & UCTXIFG0));
    }
    EUSCI_B2->CTLW0 |= UCTXSTP; //Stop
    while(!(EUSCI_B2->IFG & UCSTPIFG));
}

