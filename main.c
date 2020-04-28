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

/*--------- Macros ---------*/

#define _E(x) x
#define E(e) _E(e)

#if USE_ASM_MACRO

#define _set_bit(port, bit) __asm__ __volatile__ ("sbi %[p], %[b]":: [p] "I" _SFR_IO_ADDR(port), [b] "I" (bit))
#define _rst_bit(port, bit) __asm__ __volatile__ ("cbi %[p], %[b]":: [p] "I" _SFR_IO_ADDR(port), [b] "I" (bit))

#else

#define _set_bit(mask, bit) (mask) = ((1 << (bit)) | mask)
#define _rst_bit(mask, bit) (mask) =  (mask) & (0xff - (1 << (bit)))

#endif /* USE_ASM_MACRO */

#define __get_bit(port, bit) ((1 << (bit)) & port)

#define set_bit(P) _set_bit(P)
#define rst_bit(P) _rst_bit(P)
#define get_bit(P) _get_bit(P)


/*--------- Constants ---------*/

/*--- pins ---*/
#define A0 PORTD,3
#define A1 PORTD,4

#define B0 PORTD,5
#define B1 PORTD,6

#define C0 PORTD,7
#define C1 PORTB,3


#define E_STOP_INT PCINT10_vect
#define E_STOP     PINC,2

#define SNS_P       PIND,3
#define P_INT       INT1_vect

#define Y_A         PORTB,0
#define Y_B         PORTB,1
#define Y_C         PORTB,2
#define ERR_IND     PORTB,3
#define RUN_CONV    PORTB,3


/*--------- Function predeclaration ---------*/


/*--------- Globals ---------*/
void setup(void);

/*--------- Main ---------*/
int main(void)
{

  while(1)
    {

    }
}

/*--------- Interrupts ---------*/
ISR(EM_STOP_INT) //Emergency stop button ISR
{

}

/*--------- Function definition ---------*/
void setup(void)
{
  __asm__ __volatile__("ldi r16, 0x00   \n\t"
                       "out %[DC], r16; set portc all as input\n\t"
                       "ldi r16, 0x0c; \n\t"
                       "out %[DD], r16; "
                       "ldi r16, 0x0f; \n\t"
                       "out %[DB], r16; set portb 0,1,2 as outputs"
                       :: [DB] "I" (_SFR_IO_ADDR(DDRB)), [DC] "I" (_SFR_IO_ADDR(DDRC)), [DD] "I" (_SFR_IO_ADDR(DDRD)):"r16");
}

/*--------- EOF ---------*/
