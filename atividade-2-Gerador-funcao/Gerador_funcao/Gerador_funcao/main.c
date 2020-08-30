/**
   ----------------------------------------------------------------------------
   Copyright (c) 2020 Lucas Martins Mendes & Matheus Reibnitz  Willemann
   All rights reserved.

   Redistribution and use in source and binary forms are permitted
   provided that the above copyright notice and this paragraph are
   duplicated in all such forms and that any documentation,
   advertising materials, and other materials related to such
   distribution and use acknowledge that the software was developed
   by Lucas M. M. & Matheus R. W..
   The name of the Lucas Martins Mendes may not be used to endorse or
   promote products derived from this software without specific
   prior written permission.
   THIS SOFTWARE IS PROVIDED ''AS IS'' AND WITHOUT ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED

   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

   @author Lucas Martins Mendes &  Matheus Reibnitz  Willemann
   @file   main.c
   @date   ter abr 28 15:47:01 -03 2020
   @brief  Atividade desenvolvida para a disciplina de Eletrônica Digital 2

   @see https://mil.ufl.edu/3744/docs/lcdmanual/commands.html
   @see https://www.sparkfun.com/datasheets/LCD/HD44780.pdf
   ----------------------------------------------------------------------------
*/

/*--------- Includes ---------*/

#include <avr/io.h>

#define F_CPU  16000000UL
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>


#include "util.h"

#include "lcd.h"

/*--------- Macros ---------*/

/*--------- Constants ---------*/

/*--- pins ---*/
#define DAC0 PORTB,0
#define DAC1 PORTB,1
#define DAC2 PORTB,2
#define DAC3 PORTB,3
#define DAC4 PORTB,4
#define DAC5 PORTB,5
#define DAC6 PORTB,6
#define DAC7 PORTB,7

#define LED_ERR PORTC,0
#define LED_ON  PORTC,1
#define LED_RUN PORTC,2

/*--------- predeclaration ---------*/
typedef enum machineState {
    STOP = 0,
    RUN,
    ERROR
} machineState_t;

typedef enum cmd {
    CMD_STOP = 's',
    CMD_RUN  = 'r',
    CMD_CFG  = 'c',
    CMD_HLP  = 'h'
} cmd_t;

typedef enum wave_type {
    WAVE_SINE = 's', /*!< Sine wave */
    WAVE_SQRE = 'q', /*!< Square wave */
    WAVE_SWTT = 'w', /*!< sawtooth */
    WAVE_TRGL = 't'  /*!< trianglew */
} wave_type_t;

/*--------- Globals ---------*/

/*--------- Main ---------*/
int main(void)
{
    //set up pin directions
    DDRB = 0xff;
    DDRC = 0x07;
    DDRD = 0xff;

    __asm__("sei;");

    /* DO NOT TOUCH ABOVE THIS */
    while(1) {
        switch(major_state) {
        
        }
 
    }
    /*DO NOT TOUCH BELOW THIS*/
}

/*--------- Interrupts ---------*/
ISR(E_STOP_INTR) //Emergency stop button ISR
{
    major_state = ERROR;
    rst_bit(CYL_B);
    set_bit(CYL_C);
    rst_bit(CYL_A);

    while(!get_bit(E_STOP_BTN)); //lock the machine while the emergency button is pressed
}
ISR(PAUSE_INT)
{
    major_state = (major_state == RUN ? PAUSE : (major_state ==  PAUSE ? RUN : major_state));
    _delay_ms(10); //button debounce
}
/*--------- Function definition ---------*/

/*--------- EOF ---------*/
