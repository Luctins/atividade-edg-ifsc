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
#define DEBUG_PULSE_PIN_ISR 0

#define set_2byte_reg(val, reg) reg ## H = (val >> 8); reg ## L = (val & 0xff);

const char const * line_termination = "\r\n";
#define serial_debug(msg) uart_send_str(msg); uart_send_str(line_termination)

/*--------- Constants ---------*/
#define CMD_BUFF_LEN 127
#define BAUD_RATE (9600)
#define WAVE_PTS (100)
#define LUT_LEN (WAVE_PTS)
#define MAX_F (100)
/**
   100 Point LUT's for all minimally complex curves
*/
PROGMEM static const uint8_t sine_lut[LUT_LEN] =
    {
     127, 135, 143, 151, 159, 167, 174, 182, 189, 196, 203, 209, 215, 221, 226, 231,
     235, 239, 243, 246, 249, 251, 253, 254, 254, 254, 254, 253, 252, 250, 247, 245,
     241, 237, 233, 228, 223, 218, 212, 206, 199, 192, 185, 178, 171, 163, 155, 147,
     139, 131, 123, 115, 107, 99, 91, 83, 76, 69, 62, 55, 48, 42, 36, 31, 26, 21,
     17, 13, 9, 7, 4, 2, 1, 0, 0, 0, 0, 1, 3, 5, 8, 11, 15, 19, 23, 28, 33, 39, 45,
     51, 58, 65, 72, 80, 87, 95, 103, 111, 119, 127
    };
PROGMEM static const uint8_t trgl_lut[LUT_LEN] = 
    {
     0, 5, 10, 15, 20, 25, 30, 36, 41, 46, 51, 56, 61, 66, 72, 77, 82, 87, 92, 97,
     103, 108, 113, 118, 123, 128, 133, 139, 144, 149, 154, 159, 164, 170, 175, 180,
     185, 190, 195, 200, 206, 211, 216, 221, 226, 231, 236, 242, 247, 252, 2, 7, 12,
     18, 23, 28, 33, 38, 43, 48, 54, 59, 64, 69, 74, 79, 84, 90, 95, 100, 105, 110,
     115, 121, 126, 131, 136, 141, 146, 151, 157, 162, 167, 172, 177, 182, 188, 193,
     198, 203, 208, 213, 218, 224, 229, 234, 239, 244, 249, 255
    };
PROGMEM static const uint8_t swtt_lut[LUT_LEN] = 
    {
     0, 2, 5, 7, 10, 12, 15, 18, 20, 23, 25, 28, 30, 33, 36, 38, 41, 43, 46, 48, 51,
     54, 56, 59, 61, 64, 66, 69, 72, 74, 77, 79, 82, 85, 87, 90, 92, 95, 97, 100,
     103, 105, 108, 110, 113, 115, 118, 121, 123, 126, 128, 131, 133, 136, 139, 141,
     144, 146, 149, 151, 154, 157, 159, 162, 164, 167, 170, 172, 175, 177, 180, 182,
     185, 188, 190, 193, 195, 198, 200, 203, 206, 208, 211, 213, 216, 218, 221, 224,
     226, 229, 231, 234, 236, 239, 242, 244, 247, 249, 252, 255
    };
/*--- pins ---*/
#define DAC_PORT PORTB

#define FREQ_ADJ_POT 7 /*DAC channel 7*/
#define BTN_SS_vect INT1_vect
#define BTN_WAVE_vect INT0_vect
#define BTN_SS PIND,3 //start stop btn
#define BTN_WAVE  PIND,2 //toggle wave type button

#define LED_ON  PORTC,1
#define LED_RUN PORTC,0

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
inline void timer1_stop(void)  { TIMSK1 = 0x00; }
inline void timer1_start(void) { TIMSK1 = 0x02; }

/*--- UART ---*/
void uart_init(uint32_t baudrate);
void uart_send_char(const char c);
void uart_send_str(const char * buff);
//void USART_Transmit_string(char *data);

/*--- Others ---*/
void parse_cmd(char * _cmd_buff);
void show_status(void);

/*--------- Globals ---------*/
#if 0
static uint8_t wave_value = 0; //current position in cycle
static char is_rising = 1;
#endif

static waveType_t wave_type = WAVE_SQRE; //current generator wave type
static uint16_t lut_pos = 0; //position in lookup table, used for sine gen

static machineState_t major_state = STOP;//machine state
static uint8_t major_state_transition = 1;

uint16_t frequency = 50;
uint16_t ADCread = 512;

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

    DDRC |= 0x04;//debug pin

    //Configure pin interrupts
    EICRA = 0x00; //set both INT0 and INT1 as falling edge
    EIMSK = 0x03; //enable INT1 and INT0

    /* configure ADC  */
	ADMUX = 0b01000111; //Vref = pin AVCC (5V), ADC = pin ADC7
	ADCSRA = 0b10000110; //bit0: ADC enable, bits 2..0: prescaller
	ADCSRA |= (1 << ADSC); //starts first conversion

    /* configure timer 0 */
    TCCR0A = 0x00;
    TIMSK0 = 0x01;//enable isr for timer 0 ovf 

    /*configure timer 1 */
    TCCR1A = 0b00000000; //timer in normal mode,
    TCCR1B = 0x02; //presc = 8
    TIMSK1 = 0x00; //enable Interrupt for OC1A
    timer1_set_period_us(10);

    uart_init(BAUD_RATE);

    //enable interrupts
    __asm__("sei;");

    uart_send_str("hello there!");
    set_bit(LED_ON);

    while(1) {
        //do state transition actions or run steady state code
        if(major_state_transition) {
            switch(major_state) {
            case STOP:
            timer1_stop();
            DAC_PORT = 0x00;
            rst_bit(LED_RUN);
            break;
        case RUN:
            timer1_start();
			if (ADCSRA & (1 << ADIF)){ //if converion ended
				ADCread = ADCW; //read conversion
				ADCread = ((ADCread * 90)/1023); //0-1023 scale -> 0-90 scale
				ADCSRA |= (1 << ADSC); //starts next conversion
				
				//serial test print:
				/*char buff[6];
				snprintf(buff, 5, "%i", ADCread);
				uart_send_str(buff);*/
			}
            set_bit(LED_RUN);
            switch(wave_type) {
            case WAVE_SINE:
                set_bit(LED_SINE);
                rst_bit(LED_SWTT);
                rst_bit(LED_TRGL);
                rst_bit(LED_SQRE);
                break;
            case WAVE_TRGL:
                set_bit(LED_TRGL);
                rst_bit(LED_SINE);
                rst_bit(LED_SWTT);
                rst_bit(LED_SQRE);
                break;
            case WAVE_SQRE:
                set_bit(LED_SQRE);
                rst_bit(LED_SINE);
                rst_bit(LED_SWTT);
                rst_bit(LED_TRGL);
                break;
            case WAVE_SWTT:
                set_bit(LED_SWTT);
                rst_bit(LED_SINE);
                rst_bit(LED_TRGL);
                rst_bit(LED_SQRE);
                break;
            }
            break;
        }
        }
        else {
            switch(major_state) {
            case STOP:
                if(cmd_recved) {
                    parse_cmd(cmd_buff); //parse incoming message
                }
                break;
            case RUN:
                if (!shown_status) {
                    show_status();
                    shown_status = 1;
                }
                break;
            }
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
    major_state_transition = 1;
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
    lut_pos = 0;
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
#if DEBUG_PULSE_PIN_ISR == 1
    set_bit(DEBG_PIN);
#endif

    set_2byte_reg(0x0000, TCNT1); //reset timer value

    switch(wave_type) {
    case WAVE_SINE:
        DAC_PORT = sine_lut[lut_pos];
        lut_pos = lut_pos < LUT_LEN ? lut_pos + 1 : 0;
        break;
    case WAVE_TRGL:
        DAC_PORT = trgl_lut[lut_pos];
        lut_pos = lut_pos < LUT_LEN ? lut_pos + 1 : 0;
        break;
    case WAVE_SWTT:
        DAC_PORT = swtt_lut[lut_pos];
        lut_pos = lut_pos < LUT_LEN ? lut_pos + 1 : 0;
        break;
    case WAVE_SQRE:
        lut_pos = lut_pos < LUT_LEN ? lut_pos + 1 : 0;
        DAC_PORT = lut_pos < LUT_LEN/2 ? 0 : 255;
        break;
#if 0
    case WAVE_TRGL:
        DAC_PORT = wave_value; //TODO: FIX THIS wrong range shoulf be 0-255
        if(is_rising) {
            wave_value = wave_value < WAVE_PTS ? wave_value + 1 : 0;
            if(wave_value == WAVE_PTS) {
                is_rising = 0;
            }
        } else {
            wave_value
                = wave_value ? wave_value - 1 : WAVE_PTS;
            if(wave_value == 0) {
                is_rising = 1;
            }
        }
        break;
    case WAVE_SWTT:
        DAC_PORT = wave_value; //TODO: FIX THIS wrong range shoulf be 0-255
        wave_value = wave_value < WAVE_PTS ? wave_value + 1 : 0;
        break;
    case WAVE_SQRE:
        DAC_PORT = wave_value < WAVE_PTS/2 ? 0 : 255;
        wave_value = wave_value < WAVE_PTS ? wave_value + 1 : 0;
        break;
#endif
    }
#if DEBUG_PULSE_PIN_ISR == 1
    rst_bit(DEBG_PIN);
#endif
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
  //uart_send_char(*cmd_buff_pos); //test
}

/*--------- Function definition ---------*/
/*--- Timer1 ---*/
void timer1_set_period_us(uint16_t t_us)
{
    //test for greatest period that fits in OCreg
    t_us = t_us > (65536/2) ? (65536/2) : t_us;
    uint16_t OCval = t_us;
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
        major_state_transition = 1;
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
        major_state_transition = 1;
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
    //set_2byte_reg(((F_CPU / (16UL * baudrate) ) - 1), UBRR0);//set baudrate prescaler
    UBRR0H = (((F_CPU / ((uint32_t)16 * baudrate) ) - 1) >> 8);
    UBRR0L = ((F_CPU / ((uint32_t)16 * baudrate) ) - 1); // Modo Normal Assíncrono;
}

/*--------- EOF ---------*/


#if 0
void USART_Transmit_string( char *data ) //transmite um dado (uma string) pela serial (do professor)
{
    while(*data != '\0')
    {
		
		
    }
}
//exemple serial print
char buff[6];
snprintf(buff, 5, "%i", ADCread);
uart_send_str(buff);
#endif
