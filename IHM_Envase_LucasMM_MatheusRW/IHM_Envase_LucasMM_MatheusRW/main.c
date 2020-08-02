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
#include <string.h>

#include "util.h"

#include "lcd.h"

/*--------- Macros ---------*/

/*--------- Constants ---------*/

/*--- pins ---*/
#define CYL_A PORTC,3
#define CYL_B PORTC,4
#define CYL_C PORTC,5


#define A_O PINB,0
#define A_1 PINB,1
#define B_O PINB,2
#define B_1 PINB,3
#define C_O PINB,4
#define C_1 PINB,5

#define SNS_CX PINB,6

/*--- Buttons ---*/

#define UP_BTN PINB,7
#define DWN_BTN PINC,0
#define ENTR_BTN PINC,1

/*emergency stop button and interrupt*/
#define E_STOP_INTR  INT0_vect
#define E_STOP_BTN   PIND,2

#define STRT_STOP_BTN PINC,2

#define PAUSE_BTN PIND,3
#define PAUSE_INT INT1_vect

/*--- Default Values ---*/
#define FILL_DELAY_DEFAULT 500
#define PWD_DEFAULT "1111"
#define PWD_LEN (4)


/*--------- predeclaration ---------*/
typedef enum machineState {
    START = 0,
    PWD,
    CONFIG,
    READY,
    RUN,
    PAUSE,
    ERROR
} machineState_t;

typedef enum runState {
    WAITING = 0, /*!< esperando caixas */
    DETECTED,    /*!< caixa detectada */
    LOADING,     /*!< despejando material */
    CLOSING,     /*!< fecha CYL_C para poder liberar caixa */
    RELEASING,   /*!< libera caixa e recarrega compartimento interno */
} runState_t;

const char pwd_txt[] = "password:";

/*--------- Globals ---------*/

volatile machineState_t major_state = START;
volatile runState_t run_state = WAITING;

volatile uint32_t fill_delay_ms = FILL_DELAY_DEFAULT;

char pwd_buff[PWD_LEN+1]= "\0";

/*--------- Main ---------*/
int main(void)
{
    //configure interrupts
    EICRA |= 0b00001010; //set INT0 and INT1 as falling edge

    EIMSK |= 0x03; //enable INT 1 and 0

    //set up pin directions
    DDRB = 0x00;
    DDRC = 0b00111000;
    DDRD = 0b11110011;

    sei();

    lcd_4bit_init();

    lcd_write("Hello there!");
    _delay_ms(500);
  while(1)
    {
        switch(major_state)
        {
        case START:
            //TODO: chech initial state
            break;
        case PWD:
            lcd_clear();
            lcd_move_cursor(0,0);
            lcd_write(pwd_txt);
            //draw * equivalent to the input password len
            uint8_t curr_opt = 0;
            uint8_t pwd_len = 0;
            while(major_state == PWD)
            {
                lcd_move_cursor(pwd_len, 1);
                lcd_cmd('0' + curr_opt, LCD_CMD);
                if(get_bit(UP_BTN))
                {
                    curr_opt = curr_opt >= 9 ? 0 : curr_opt + 1;
                    _delay_ms(50);
                }
                if(get_bit(DWN_BTN))
                {
                    curr_opt = curr_opt == 0 ? 9 : curr_opt - 1;
                    _delay_ms(50);
                }
                if(get_bit(ENTR_BTN))
                {
                    if(pwd_len == 4)
                    {
                        if(strncmp(PWD_DEFAULT, pwd_buff, 4) == 0) //check password match
                        {
                            major_state = CONFIG;
                            break;
                        }
                        else //wrong password
                        {
                            lcd_move_cursor(0, 1);
                            lcd_write("wrong passwd");
                            _delay_ms(500);
                            lcd_move_cursor(0, 1);
                            lcd_write("                ");
                            pwd_len = 0;
                        }
                    }
                    else
                    {
                        pwd_buff[pwd_len] = '0' + curr_opt;
                    }
                }
            }
        case CONFIG:
            //TODO: ask for lot size, fill_delay

        case READY:

            break;
        case RUN:
            switch(run_state) //TODO: prever casos impossíveis / erros, limpar lcd antes de escrever as coisas novas
            {
            case WAITING:
                lcd_clear();
                //lcd_write("Waiting for next box");
                if(get_bit(SNS_CX)==0) {
                    run_state = DETECTED;
                }
                break;
            case DETECTED:
                lcd_clear();
                //lcd_write("Box detected");
                set_bit(CYL_A);
                set_bit(CYL_B);
                if(get_bit(A_1)==0 && get_bit(B_1)==0) {
                    run_state = LOADING;
                }
                break;
            case LOADING:
				//lcd_write("Loading box");
				rst_bit(CYL_C);
				//_delay_ms(variable * 1000);  TODO: create variable for user-defined wait time (seconds)
				if(get_bit(C_O)==0) run_state = RELEASING;
                break;
			case CLOSING:
				//lcd_write("Closing dispenser");
				set_bit(CYL_C);
				if(get_bit(C_1)==0) run_state = RELEASING;
				break;
            case RELEASING:
				//lcd_write("Releasing box");
				set_bit(CYL_A);
				set_bit(CYL_B);
				if(get_bit(A_1)==0 && get_bit(B_1)==0) run_state = LOADING;
                break;
            }
            break;
        case PAUSE:
			//lcd_write("System paused...");
            break;
        default:
        case ERROR:

			//lcd_write("SYSTEM ERROR");
			break;
        }
    }
}

/*--------- Interrupts ---------*/
ISR(E_STOP_INTR) //Emergency stop button ISR
{
   major_state = ERROR;
	set_bit(CYL_B);
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
