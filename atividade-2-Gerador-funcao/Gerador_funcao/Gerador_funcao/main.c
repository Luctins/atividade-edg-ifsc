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
   @date   Sun 30 Aug 16:44:10 -03 2020
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


/*--------- Macros ---------*/

#define set_2byte_reg(val, reg) reg ## H = (val >> 8); reg ## L = (val & 0xff);

const char * line_termination = "\r\n";
#define serial_debug(msg) serial_send(msg); serial_send(line_termination)

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
#define DAC_PORT PORTB

#define LED_ERR PORTC,0
#define LED_ON  PORTC,1
#define LED_RUN PORTC,2

/*--- parameters ---*/
#define BAUD_RATE (115200)
#define BAUD_PRESCALE ((F_CPU / (16UL * BAUD_RATE) ) - 1)

/*--------- predeclaration ---------*/

/*--- Timer 1 ---*/
void timer1_set_period_us(uint16_t t_us);
/**
  Stop timer 1.
  @return void
*/
inline void timer1_stop(void) { set_reg(TCCR1B, 0x07, 0x00); }

/**
  Start timer 1 with prescaler of 1.
  @return void
*/
inline void timer1_start(void) { set_reg(TCCR1B, 0x07, 0x01); }

/*--- Serial ---*/
void serial_send_char(const char c);
void serial_send(const char * buff);
void serial_init(void);
void show_status(void);

/*--- Others ---*/
void parse_cmd(char * _cmd_buff);

/*--- Types ---*/
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

typedef enum waveType {
    WAVE_SINE = 's', /*!< Sine wave */
    WAVE_SQRE = 'q', /*!< Square wave */
    WAVE_SWTT = 'w', /*!< sawtooth */
    WAVE_TRGL = 't'  /*!< triangle */
} waveType_t;


/*--------- Globals ---------*/
static uint8_t wave_value = 0;

static waveType_t wave_type = WAVE_SWTT;

/**
   100 Point LUT
 */
static uint8_t sine_lut[] =
{ 127, 135, 143, 151, 159, 166, 174, 181, 188, 195, 202, 208, 214, 220, 225,
  230, 235, 239, 242, 246, 248, 250, 252, 253, 254, 255, 254, 253, 252, 250,
  248, 246, 242, 239, 235, 230, 225, 220, 214, 208, 202, 195, 188, 181, 174,
  166, 159, 151, 143, 135, 127, 119, 111, 103, 95, 88, 80, 73, 66, 59, 52,
  46, 40, 34, 29, 24, 19, 15, 12, 8, 6, 4, 2, 1, 0, 0, 0, 1, 2, 4, 6, 8, 12,
  15, 19, 24, 29, 34, 40, 46, 52, 59, 66, 73, 80, 88, 95, 103, 111, 119 };

static uint16_t lut_pos = 0;

static machineState_t major_state = STOP;

char cmd_buff[128];
static char is_rising = 1;

uint16_t frequency = 10;

/*--------- Main ---------*/
int main(void)
{
    //set up pin directions
    DDRB = 0xff;
    DDRC = 0x07;
    DDRD = 0xff;
    
	//
	serial_init();
	
    //configure timer 1
    TCCR1A = 0b00000000; //timer in normal mode, no PWM modes
    TIMSK1 = 0x02; //enable Interrupt for OC1A

    //enable interrupts
    __asm__("sei;");

    /* DO NOT TOUCH ABOVE THIS */
	set_bit(LED_ON);
  while(1) {


      switch(major_state) {
			case STOP:
          break;
			case RUN:
          show_status(); //TODO: show status every half a second (no delays)
          break;
			case ERROR:
          
          break;
      }
      //parse incoming characters
      parse_cmd(cmd_buff);
    }
    /*DO NOT TOUCH BELOW THIS*/
}

/*--------- Interrupts ---------*/
ISR(TIMER1_COMPA_vect)
{
    //reset timer value
    set_2byte_reg(0x0000, TCNT1);

    switch(wave_type) {
    case WAVE_SINE:
        DAC_PORT = sine_lut[lut_pos];
        lut_pos = lut_pos >= 1000 ? 0 : lut_pos + 1;
        break;
    case WAVE_TRGL:
        DAC_PORT = wave_value;
        if(is_rising) {
            ++wave_value;
            if(wave_value == 255) {
                is_rising = 0;
            }
        } else {
            --wave_value;
            if(wave_value == 0) {
                is_rising = 1;
            }
        }
        break;
    case WAVE_SWTT:
        DAC_PORT = wave_value++; //yes, it should overflow
        break;
    case WAVE_SQRE:
        DAC_PORT = wave_value < 127 ? 0 : 255;
        ++wave_value;
        break;
    }
}

ISR(USART_RX_vect) //recepção serial
{
	char buff;
	buff = UDR0; //dado que vem pela serial, manda bite a bite

	//TODO: configurar recepção serial aqui
}

/*--------- Function definition ---------*/
void timer1_set_period_us(uint16_t t_us)
{
    //test for greatest period that fits in OCreg
    t_us = t_us > 16000 ? 16000 : t_us;
    uint16_t OCval = t_us*16;
    //set output compare high and low byte
    set_2byte_reg(OCval, OCR1A);
    //OCR1AH = (OCVal >> 8);
    //OCR1AL = (OCval & 0xff);
}

void parse_cmd(char * _cmd_buff)
{
    const char help_str[] =
        "-------------------------------------------------------\n"
        "h - help\n"
        "r - run generator (plase configure first)\n"
        "s - stop generator\n"
        "c - configure generator - format: c <waveType> <freq>\n"
        "\t wavetypes :\n"
        "\t  - s - [s]ine\n"
        "\t  - q - s[q]uare\n"
        "\t  - w - sa[w]tooth\n"
        "\t  - t - [t]triangle\n"
        "\t frequency: 10-100 Hz, integer"
        "-------------------------------------------------------\n";

    cmd_t cmd = _cmd_buff[0];
    char w = 0;
    uint16_t f = 9999;
    switch(cmd) {
    case CMD_RUN:
        timer1_start();
        major_state = RUN;
        set_bit(LED_RUN);
        rst_bit(LED_ERR);
    case CMD_CFG:
        sscanf(_cmd_buff, "%*c %c %u\n", &w, &f);
        if(w && f <= 1000) {
            wave_type = w;
            frequency = f;
            serial_debug("ok");
        } else {
            serial_debug("invalid arg");
        }
    case CMD_STOP:
        timer1_stop();
        serial_debug("stopped");
    default:
        serial_debug("invalid cmd");
    case CMD_HLP:
        serial_send(help_str);

        set_bit(LED_ERR);
        break;
    }
}

void serial_send(const char * buff)
{
    //increment pointer until find null byte
    for(;*buff;++buff)
    {
        serial_send_char(*buff);
    }
}

void serial_send_char(const char c)
{
    //TODO: send character here
}

void USART_Transmit_string( char *data ) //transmite um dado (uma string) pela serial (do professor)
{
	while(*data != '\0')
	{
		/* Wait for empty transmit buffer */
		while ( !(UCSR0A & (1<<UDRE0)));
		
		/* Put data into buffer, sends the data */
		UDR0 = *data++;
	}
}


void serial_init(void)
{
	// Configure serial, 9600bps, 8bits per frame
	UCSR0B = (1 << TXEN0)|(1 << RXEN0); //Enable serial
	UCSR0B |= (1 << RXCIE0); //Enable serial reception on interruption
	UCSR0C = (1 << UCSZ00) | (1 << UCSZ01); //8bits per frame
	UBRR0H = (BAUD_PRESCALE >> 8); //9600bps
	UBRR0L = BAUD_PRESCALE; // Modo Normal Assíncrono;
}

void show_status(void)
{
    char buff[150];
    // 53 = len of status line
    const char * wave_art[] =
        {
            "|  _\r\n"
            "| /  \\\r\n"
            "-/----\\---/->\r\n"
            "|      \\_/\r\n",
            "| __\r\n"
            "||  |\r\n"
            "-|--|--|->\r\n"
            "|   |__|\r\n",
            "|\r\n"
            "| /|  /|\r\n"
            "-/-|-/-|->\r\n"
            "|  |/\r\n",
            "|\r\n"
            "| /\\\r\n"
            "-/--\\--/->\r\n"
            "|    \\/\r\n",
        };
    snprintf(buff, 150,
             "-----------\r\n"
             "status: %c wavef: %c freq: %03i\r\n"
             "%s"
             "-----------\r\n", major_state == RUN ? 'r' : 's', wave_type, frequency,
             (wave_type == WAVE_SINE ? wave_art[0] :
              (wave_type == WAVE_SQRE ? wave_art[1] :
               (wave_type == WAVE_SWTT ? wave_art[2] :
                (wave_art[3])))));
    serial_send(buff);
    for (uint8_t i = strnlen(buff,150); i; --i) {
        serial_send_char(127); //send ascii del
    };
}
/*--------- EOF ---------*/
