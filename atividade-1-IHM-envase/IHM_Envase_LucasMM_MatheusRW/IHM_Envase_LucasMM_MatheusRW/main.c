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
#define CYL_A PORTC,3
#define CYL_B PORTC,4
#define CYL_C PORTC,5


#define A_0 PINB,0
#define A_1 PINB,1
#define B_0 PINB,2
#define B_1 PINB,3
#define C_0 PINB,4
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

#define LOT_SIZE_DEFAULT 3
#define LOT_QUANTITY_DEFAULT 0
#define LOT_NUMBER_DEFAULT 1

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

const char pwd_txt[] = "Password:";


/*
  static void drawIdle()
  {
  const char * l = "|/-\\";
  static char * c = l;

  lcd_move_cursor(0xf, 0);
  lcd_cmd(*c, cmdType_t type);
  }
*/
/*--------- Globals ---------*/

volatile machineState_t major_state = START;
volatile runState_t run_state = WAITING;

volatile uint32_t fill_delay_ms = FILL_DELAY_DEFAULT;

char pwd_buff[PWD_LEN+1]= "\0";

uint8_t lot_size = LOT_SIZE_DEFAULT;            //Caixas por lote (definida na config)
uint8_t lot_quantity = LOT_QUANTITY_DEFAULT;    //Caixas prontas no lote atual
uint8_t lot_number = LOT_NUMBER_DEFAULT;        //Número do lote (quantos lotes já foram feitos)

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
    __asm__("sei;");
    //sei();

    lcd_4bit_init();

    lcd_write("Booting");
    _delay_ms(200);
    lcd_write(".");
    _delay_ms(200);
    lcd_write(".");
    _delay_ms(200);
    lcd_write(".");
    _delay_ms(200);

    while(1) {
        switch(major_state) {
        case START:
            run_state = WAITING;
            rst_bit(CYL_A);
            rst_bit(CYL_B);
            set_bit(CYL_C);
            lcd_move_cursor(0,0);
            lcd_write("Wait start pos.");
            if(!(get_bit(A_0) || get_bit(B_0) || get_bit(C_1))) {
                major_state = PWD;
            }
            break;
        case PWD:
            lcd_clear();
            uint8_t curr_opt = 0;
            uint8_t pwd_pos = 0;
            memcpy(pwd_buff, "0   \0", 5);
            while(1)
            {
                lcd_move_cursor(0,0);
                lcd_write(pwd_txt);
                lcd_move_cursor(0, 1);
                pwd_buff[pwd_pos] = '0' + curr_opt;
                lcd_write(pwd_buff);
                while(get_bit(UP_BTN) && get_bit(DWN_BTN) && get_bit(ENTR_BTN)) {
                    //draw_idle();
                }

                if(!get_bit(UP_BTN)) {
                    curr_opt = curr_opt >= 9 ? 0 : curr_opt + 1;
                    _delay_ms(50);
                    while(!get_bit(UP_BTN)); //wait button release
                } else if (!get_bit(DWN_BTN)) {
                    curr_opt = curr_opt == 0 ? 9 : curr_opt - 1;
                    _delay_ms(50);
                    while(!get_bit(DWN_BTN)); //wait button release
                } else if (!get_bit(ENTR_BTN)) {
                    if(pwd_pos == PWD_LEN-1)
                    {
                        //check password match
                        if(strncmp(PWD_DEFAULT, pwd_buff, 4) == 0) {
                            major_state = CONFIG;
                            lcd_clear();
                            break;
                        }
                        //wrong password
                        else {
                            lcd_move_cursor(0, 1);
                            lcd_write("Wrong passwd");
                            _delay_ms(1000);
                            lcd_clear();
                            lcd_move_cursor(0, 1);
                            memcpy(pwd_buff, "0   \0", 5);
                            pwd_pos = 0;
                        }
                    }
                    else {
                        //pwd_buff[pwd_pos] = '0' + curr_opt;
                        ++pwd_pos;
                    }
                    while(!get_bit(ENTR_BTN)); //wait for button release
                    _delay_ms(50);
                }
            }
        case CONFIG:
            lcd_move_cursor(0,0);
            lcd_write("Conf. param.");
            _delay_ms(500);
            lcd_clear();
            char n_buff[5];
            char ok = 0;
            do
            {
                lcd_move_cursor(0,0);
                lcd_write("Cycle Count:");
                lcd_move_cursor(0, 1);
                snprintf(n_buff, 4, "%02i", lot_size);
                lcd_write(n_buff);

                while(get_bit(UP_BTN) && get_bit(DWN_BTN) && get_bit(ENTR_BTN)) {
                    //draw_idle();
                }
                // increment lot amount
                if(!get_bit(UP_BTN)) {
                    lot_size = lot_size >= 24 ? 1 : lot_size + 1;
                    _delay_ms(50);
                    while(!get_bit(UP_BTN)); //wait button release
                }
                //decrement lot size
                else if (!get_bit(DWN_BTN)) {
                    lot_size = lot_size == 1 ? 24 : lot_size - 1;
                    _delay_ms(50);
                    while(!get_bit(DWN_BTN)); //wait button release
                }
                //confirm
                else if (!get_bit(ENTR_BTN)) {
                    ok = 1;
                    _delay_ms(50);
                    while(!get_bit(ENTR_BTN)); //wait for button release
                }
            } while(!ok);
            ok = 0;
            char fill_delay = 1;
            lcd_clear();
            do {
                lcd_move_cursor(0,0);
                lcd_write("Delay:");
                lcd_move_cursor(0, 1);
                snprintf(n_buff, 5, "%02i s", fill_delay);
                lcd_write(n_buff);

                while(get_bit(UP_BTN) && get_bit(DWN_BTN) && get_bit(ENTR_BTN)) {
                    //draw_idle();
                }
                // increment lot amount
                if(!get_bit(UP_BTN)) {
                    fill_delay = fill_delay >= 99 ? 1 : fill_delay + 1;
                    _delay_ms(50);
                    while(!get_bit(UP_BTN)); //wait button release
                }
                //decrement lot size
                else if (!get_bit(DWN_BTN)) {
                    fill_delay = fill_delay == 1 ? 99 : fill_delay - 1;
                    _delay_ms(50);
                    while(!get_bit(DWN_BTN)); //wait button release
                }
                //confirm
                else if (!get_bit(ENTR_BTN)) {
                    ok = 1;
                    fill_delay_ms = 1000 * fill_delay;
                    _delay_ms(50);
                    while(!get_bit(ENTR_BTN)); //wait for button release
                }
            } while(!ok);
            lcd_clear();
            major_state=READY;
        case READY:
            lcd_move_cursor(0,0);
            lcd_write("Ready press STR");
            if (get_bit(STRT_STOP_BTN)==0) {
                major_state=RUN;
            }
            break;
        case RUN:
            ;
            //Segunda linha do LCD, status do lote:
            char buff[17];
            snprintf(buff,17, "Lot %02i, box %02i ",lot_number,lot_quantity+1);
            lcd_move_cursor(0,1);
            lcd_write(buff);
            switch(run_state) {
            case WAITING:
                lcd_move_cursor(0,0);
                lcd_write("Waiting box    ");
                if(get_bit(SNS_CX)==0) {
                    run_state = DETECTED;
                }
                break;
            case DETECTED:
                lcd_move_cursor(0,0);
                lcd_write("Box detected   ");
                set_bit(CYL_A);
                set_bit(CYL_B);
                if(get_bit(A_1)==0 && get_bit(B_1)==0) {
                    run_state = LOADING;
                }
                break;
            case LOADING:
                rst_bit(CYL_C);
			lcd_move_cursor(0,0);
				lcd_write("Loading box... ");
              if(get_bit(C_0)==0) {
                    run_state = CLOSING;
					lcd_move_cursor(0,0);
					lcd_write("Applying delay ");
					for(int i =0; i<fill_delay_ms/10; ++i)
					{
						_delay_ms(10);
					}
					lcd_move_cursor(0,0);
					lcd_write("Box loaded     ");
                }
                break;
            case CLOSING:
                lcd_move_cursor(0,0);
                lcd_write("Closing disp.  ");
                set_bit(CYL_C);
                if(get_bit(C_1)==0) {
                    run_state = RELEASING;
                }
                break;
            case RELEASING:
                lcd_move_cursor(0,0);
                lcd_write("Releasing box  ");
                rst_bit(CYL_A);
                rst_bit(CYL_B);
                if(get_bit(A_0)==0 && get_bit(B_0)==0) {
                    lcd_move_cursor(0,0);
                    lcd_write("Box finished   ");
                    _delay_ms(2000);
                    ++ lot_quantity; //Incrementa uma caixa no lote atual
                    if (lot_quantity == lot_size) //Se o lote atual atingiu o número de caixas desejado
                    {
                        ++ lot_number; //Incrementa número de lotes prontos
                        lot_quantity = 0; //Reinicia contagem de caixas no lote
                        lcd_move_cursor(0,0);
                        lcd_write("Lot finished   ");
                        _delay_ms(1000);
                        lcd_move_cursor(0,0);
                        lcd_write("Start next lot ");
                        _delay_ms(1000);
						major_state = READY;
                    }
                    run_state = WAITING;
                }
                break;
            }
            break;
        case PAUSE:
            lcd_move_cursor(0,0);
            lcd_write("System paused..");
            break;
        default:
        case ERROR:
			rst_bit(CYL_A);
			rst_bit(CYL_B);
			set_bit(CYL_C);
            lcd_clear();
            lcd_write("SYSTEM ERROR");
            break;
        }
        /*
          Situações impossíveis dos cilindros:
          Se os dois sensores de um cilindro estiverem acionados ao mesmo tempo,
          ou se B e C estiverem abertos ao mesmo tempo (vazamento)
        */
        if ((get_bit(A_0) == 0 && get_bit(A_1) == 0) ||
            (get_bit(B_0) == 0 && get_bit(B_1) == 0) ||
            (get_bit(C_0) == 0 && get_bit(C_1) == 0) ||
            (get_bit(B_0)==0 && get_bit(C_0) == 0)) {

            major_state = ERROR;
        }
    }
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
