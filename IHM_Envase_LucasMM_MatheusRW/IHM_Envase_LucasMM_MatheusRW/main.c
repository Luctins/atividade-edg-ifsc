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
   ----------------------------------------------------------------------------
*/

/*--------- Includes ---------*/

#include <avr/io.h>

#define F_CPU  16000000UL
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define USE_ASM_MACRO 1
#include "util.h"

#include "lcd.h"

/*--------- Macros ---------*/


/*--------- Constants ---------*/

/*--- pins ---*/
#define CYL_A PORTD,2
#define CYL_B PORTD,3
#define CYL_C PORTC,5

/*emergency stop button and interrupt*/
#define E_STOP_INTR PCINT12_vect
#define E_STOP      PINC,4

#define A_O PINB,0
#define A_1 PINB,1
#define B_O PINB,2
#define B_1 PINB,3
#define C_O PINB,4
#define C_1 PINB,5

#define SNS_CX PINB,6

#define UP_BTN PINB,7
#define DWN_BTN PINC,0
#define ENTR_BTN PINC,1
#define STRT_STOP_BTN PINC,2
#define PAUSE_BTN PINC,3


/*--------- predeclaration ---------*/
typedef enum machineState
{
    IDLE = 0
} machineState_t;

/*--------- Globals ---------*/
void setup(void);

/*--------- Main ---------*/
int main(void)
{
    //TODO: config reg interrupção

    lcd_4bit_init();
    lcd_write("hello, World!");

  while(1)
    {

    }
}

/*--------- Interrupts ---------*/
ISR(E_STOP_INTR) //Emergency stop button ISR
{
	//[set state as emergency] goes here
	//set_bit(CYL_B);
	//set_bit(CYL_C);
	//rst_bit(CYL_A);
}

/*--------- Function definition ---------*/

/*--------- EOF ---------*/
