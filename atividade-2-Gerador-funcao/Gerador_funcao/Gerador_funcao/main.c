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

const char const * line_termination = "\r\n";
#define serial_debug(msg) uart_send_str(msg); uart_send_str(line_termination)

/*--------- Constants ---------*/
#define CMD_BUFF_LEN 127
#define BAUD_RATE (115200)
#define WAVE_PTS (100)
#define SIN_LUT_LEN (WAVE_PTS)
#define MAX_F 100
/**
   100 Point LUT
*/
static const uint8_t sine_lut[SIN_LUT_LEN] =
{ 127, 135, 143, 151, 159, 166, 174, 181, 188, 195, 202, 208, 214, 220, 225,
  230, 235, 239, 242, 246, 248, 250, 252, 253, 254, 255, 254, 253, 252, 250,
  248, 246, 242, 239, 235, 230, 225, 220, 214, 208, 202, 195, 188, 181, 174,
  166, 159, 151, 143, 135, 127, 119, 111, 103, 95, 88, 80, 73, 66, 59, 52,
  46, 40, 34, 29, 24, 19, 15, 12, 8, 6, 4, 2, 1, 0, 0, 0, 1, 2, 4, 6, 8, 12,
  15, 19, 24, 29, 34, 40, 46, 52, 59, 66, 73, 80, 88, 95, 103, 111, 119 };

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

#define FREQ_ADJ_POT 7 /*DAC channel 7*/
#define BTN_SS_vect INT1_vect
#define BTN_WAVE_vect INT0_vect
#define BTN_SS PIND,3 //start stop btn
#define BTN_WAVE  PIND,2 //toggle wave type button

#define LED_ON  PORTC,0
#define LED_RUN PORTC,1

#define LED_SINE PORTD,7
#define LED_TRGL PORTD,6
#define LED_SQRE PORTD,5
#define LED_SWTT PORTD,4

#define DEBG_PIN PORTC,2
/*--------- predeclaration ---------*/

/*--- Types ------*/
typedef enum machineState {
    STOP = 0,
    RUN,
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

/*--- Functions ---*/
/*--- Timer 1 ---*/
void timer1_set_period_us(uint16_t t_us);
inline void timer1_stop(void)  { set_reg(TCCR1B, 0x07, 0x00); }
inline void timer1_start(void) { set_reg(TCCR1B, 0x07, 0x02); }

/*--- UART ---*/
void uart_init(uint32_t baudrate);
void uart_send_char(const char c);
void uart_send_str(const char * buff);
//void USART_Transmit_string(char *data);

/*--- Others ---*/
void parse_cmd(char * _cmd_buff);
void show_status(void);

/*--------- Globals ---------*/
static uint8_t wave_value = 0; //current position in cycle
static waveType_t wave_type = WAVE_SWTT; //current generator wave type
static uint16_t lut_pos = 0; //position in lookup table, used for sine gen

static machineState_t major_state = RUN;//machine state

static char is_rising = 1;
uint16_t frequency = 50;

char shown_status = 0;

static uint8_t t0_cnt = 0; //timer0 interrupt counter 

uint8_t cmd_recved = 0;

char cmd_buff[CMD_BUFF_LEN];
char * cmd_buff_pos=cmd_buff;
/*--------- Main ---------*/
int main(void)
{
    /* set up pin directions */
    DDRB = 0xff;
    DDRC = 0x07;
    DDRD = 0xf0;

    DDRC |= 0x04;
    //Configure pin interrupts
    EICRA = 0x00; //set both INT0 and INT1 as falling edge

    /* configure ADC  */

    /* configure timer 0 */
    TCCR0A = 0x00;
    TIMSK0 = 0x01;//enable isr for timer 0 ovf 

    /*configure timer 1 */
    TCCR1A = 0b00000000; //timer in normal mode, presc = 8
    TIMSK1 = 0x02; //enable Interrupt for OC1A
    timer1_set_period_us(10);
    timer1_start();

    uart_init(BAUD_RATE);

    //enable interrupts
    __asm__("sei;");

    uart_send_str("hello there!");
    set_bit(LED_ON);

    while(1) {		
        switch(major_state) {

        case STOP:
            timer1_stop();
            DAC_PORT = 0x00;
            if(cmd_recved) {
                parse_cmd(cmd_buff); //parse incoming message
            }
            break;
        case RUN:
            timer1_start();
            set_bit(LED_RUN);
            switch(wave_type) {
            case WAVE_SINE:
                rst_bit(LED_SINE);
                set_bit(LED_TRGL);
                break;
            case WAVE_TRGL:
                rst_bit(LED_TRGL);
                set_bit(LED_SQRE);
                break;
            case WAVE_SQRE:
                rst_bit(LED_SQRE);
                set_bit(LED_SWTT);
                break;
            case WAVE_SWTT:
                rst_bit(LED_SWTT);
                set_bit(LED_SINE);
                break;
            }
            if (!shown_status) {
                show_status();
                shown_status = 1;
            }
            break;
        }
    }
}

/*--------- Interrupts ---------*/
/**
  start stop button interrupt.
*/
ISR(BTN_SS_vect)
{
    major_state = major_state == RUN ? STOP : RUN;
    //while(get_bit(BTN_SS)); //wait button release;
}

/**
  Toggle waveformat types.
*/
ISR(BTN_WAVE_vect)
{
    switch(wave_type) {
    case WAVE_SINE:
        wave_type = WAVE_TRGL;
        break;
    case WAVE_TRGL:
        wave_type = WAVE_SQRE;
        break;
    case WAVE_SQRE:
        wave_type = WAVE_SWTT;
        break;
    case WAVE_SWTT:
        wave_type = WAVE_SINE;
        break;
    }
}

/**
   used to periodically (~100 ms) show a status line on the uart. (only sets a
   flag to show later)
*/
ISR(TIMER0_OVF_vect)
{
    if(t0_cnt++ > 10) {
        t0_cnt = 0;
        shown_status = 0;
    }
}

/**
  Update output waveform.
*/
ISR(TIMER1_COMPA_vect)
{
    //set_bit(DEBG_PIN);
    set_2byte_reg(0x0000, TCNT1); //reset timer value

    switch(wave_type) {
    case WAVE_SINE:
        DAC_PORT = sine_lut[lut_pos];
        lut_pos = lut_pos >= SIN_LUT_LEN ? 0 : lut_pos + 1;
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
    //rst_bit(DEBG_PIN);
}

/**
  handle serial data.
*/
ISR(USART_RX_vect) //recepção serial
{
  *cmd_buff_pos = UDR0; //save incoming char to buffer;
  //test buffer boundary
  if (cmd_buff_pos + 1 >= (cmd_buff+CMD_BUFF_LEN)) {
      cmd_buff_pos = cmd_buff; //discard input
      return;
  }
  //test for end of command
	if(*cmd_buff_pos == '\n') {
      *(cmd_buff_pos+1) = 0; //null terminate string
      cmd_recved = 1;
  }
  ++cmd_buff_pos;
}

/*--------- Function definition ---------*/
/*--- Timer1 ---*/
void timer1_set_period_us(uint16_t t_us)
{
    //test for greatest period that fits in OCreg
    t_us = t_us > (65536/2) ? (65536/2) : t_us;
    uint16_t OCval = t_us*2;
    //set output compare high and low byte
    set_2byte_reg(OCval, OCR1A);
    //OCR1AH = (OCVal >> 8);
    //OCR1AL = (OCval & 0xff);
}

/*--------- User interaction ---------*/
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
        major_state = RUN;
    case CMD_CFG:
        sscanf(_cmd_buff, "%*c %c %u\n", &w, &f);
        if(w && f <= 100) {
            f= f ? 1 : f;
            wave_type = w;
            frequency = f;
            /*
              The timer frequency is 100 * f, because of 100 samples per cycle
             */
            timer1_set_period_us(100/f);
            serial_debug("ok");
        } else {
            serial_debug("invalid arg");
        }
    case CMD_STOP:
        major_state = STOP;
        serial_debug("stopped");
    default:
        serial_debug("invalid cmd");
    case CMD_HLP:
        serial_debug(help_str);

        //set_bit(LED_ERR);
        break;
    }
}

void show_status(void)
{
    const uint8_t bufflen = 150;
    char buff[bufflen];

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
    snprintf(buff, bufflen,
             "-----------\r\n"
             "status: %c wavef: %c freq: %03i\r\n"
             "%s"
             "-----------\r\n",
             major_state == RUN ? 'r' : 's', wave_type, frequency,
             (wave_type == WAVE_SINE ? wave_art[0] :
              (wave_type == WAVE_SQRE ? wave_art[1] :
               (wave_type == WAVE_SWTT ? wave_art[2] :
                (wave_art[3])))));
    serial_debug(buff);
    for (uint8_t i = strnlen(buff,bufflen-2) + 2; i; --i) {
        uart_send_char(127); //send ascii del
    };
}
/*--------- UART ---------*/
void uart_send_str(const char * buff)
{
    //increment pointer until found null byte
    for(;*buff;++buff)
    {
        uart_send_char(*buff);
    }
}

void uart_send_char(const char c)
{
		/* Wait for empty transmit buffer */
    while (!(UCSR0A & (1<<UDRE0)));
		UDR0 = c;
}

void uart_init(uint32_t baudrate)
{
	// Configure uart
    UCSR0B = (1 << TXEN0)|(1 << RXEN0); //Enable serial
    UCSR0B |= (1 << RXCIE0); //Enable serial reception on interruption
    UCSR0C = (1 << UCSZ00) | (1 << UCSZ01); //8bits per frame
    set_2byte_reg(((F_CPU / (16UL * baudrate) ) - 1), UBRR0);//set baudrate prescaler
    //UBRR0H = (((F_CPU / (16UL * baudrate) ) - 1) >> 8);
    //UBRR0L = ((F_CPU / (16UL * baudrate) ) - 1);; // Modo Normal Assíncrono;
}

/*--------- EOF ---------*/


#if 0
void USART_Transmit_string( char *data ) //transmite um dado (uma string) pela serial (do professor)
{
    while(*data != '\0')
    {
		
		
    }
}
#endif
