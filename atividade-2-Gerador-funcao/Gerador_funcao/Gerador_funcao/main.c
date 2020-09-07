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
#define DEBUG_PULSE_PIN_ISR 1
#define USE_PROGMEM 1

#if USE_PROGMEM == 1
#define TAB_ALLOC PROGMEM
#else
#define TAB_ALLOC
#endif

#define set_2byte_reg(val, reg) reg ## H = (val >> 8); reg ## L = (val & 0xff);

#define serial_debug(msg) uart_send_str(msg); uart_send_char('\r'); uart_send_char('\n');

/*--------- Constants ---------*/
#define CMD_BUFF_LEN 50
#define BAUD_RATE (9600)
#define WAVE_PTS (100)
#define LUT_LEN (WAVE_PTS)
#define MAX_F (100)
/**
   100 Point LUT's for all curves except the square.
*/
static const uint8_t sine_lut[LUT_LEN] TAB_ALLOC =
{
    127, 135, 143, 151, 159, 166, 174, 181, 188, 195, 202, 208, 214, 220, 225, 230,
    235, 239, 242, 246, 248, 250, 252, 253, 254, 255, 254, 253, 252, 250, 248, 246,
    242, 239, 235, 230, 225, 220, 214, 208, 202, 195, 188, 181, 174, 166, 159, 151,
    143, 135, 127, 119, 111, 103, 95, 88, 80, 73, 66, 59, 52, 46, 40, 34, 29, 24,
    19, 15, 12, 8, 6, 4, 2, 1, 0, 0, 0, 1, 2, 4, 6, 8, 12, 15, 19, 24, 29, 34, 40,
    46, 52, 59, 66, 73, 80, 88, 95, 103, 111, 119
};
static const uint8_t trgl_lut[LUT_LEN] TAB_ALLOC =
{
    0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 51, 56, 61, 66, 71, 76, 81, 86, 91, 96,
    102, 107, 112, 117, 122, 127, 132, 137, 142, 147, 153, 158, 163, 168, 173, 178,
    183, 188, 193, 198, 204, 209, 214, 219, 224, 229, 234, 239, 244, 249, 255, 249,
    244, 239, 234, 229, 224, 219, 214, 209, 204, 198, 193, 188, 183, 178, 173, 168,
    163, 158, 153, 147, 142, 137, 132, 127, 122, 117, 112, 107, 101, 96, 91, 86,
    81, 76, 71, 66, 61, 56, 50, 45, 40, 35, 30, 25, 20, 15, 10, 5
};
static const uint8_t swtt_lut[LUT_LEN] TAB_ALLOC =
{
    0, 2, 5, 7, 10, 12, 15, 17, 20, 22, 25, 28, 30, 33, 35, 38, 40, 43, 45, 48, 51,
    53, 56, 58, 61, 63, 66, 68, 71, 73, 76, 79, 81, 84, 86, 89, 91, 94, 96, 99, 102,
    104, 107, 109, 112, 114, 117, 119, 122, 124, 127, 130, 132, 135, 137, 140,
    142, 145, 147, 150, 153, 155, 158, 160, 163, 165, 168, 170, 173, 175, 178, 181,
    183, 186, 188, 191, 193, 196, 198, 201, 204, 206, 209, 211, 214, 216, 219, 221,
    224, 226, 229, 232, 234, 237, 239, 242, 244, 247, 249, 252
};

/*--- pins ---*/
/*
  since there isn't a single PORT with enought bits to use the DAC, we've split it
  between 2 ports (as "nibbles") on the lower bits
*/
#define DAC_LN PORTB
#define DAC_LN_MASK (0x0f)
#define DAC_HN PORTC
#define DAC_HN_MASK (0x0f)

#define FREQ_ADJ_POT 7 /*DAC channel 7*/
#define DEBG_PIN PORTC,4

/*--- Buttons ---*/
#define BTN_SS_vect   INT1_vect
#define BTN_WAVE_vect INT0_vect
#define BTN_SS PIND,3 //start stop btn
#define BTN_WAVE  PIND,2 //toggle wave type button

#define LED_RUN PORTB,4
#define LED_ON  PORTB,5
#define LED_SINE PORTD,7
#define LED_TRGL PORTD,6
#define LED_SQRE PORTD,5
#define LED_SWTT PORTD,4

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

/*--- Others ---*/
void parse_cmd(char * _cmd_buff);
void show_status(void);

/*--------- Globals ---------*/

/*--- states ---*/
static machineState_t major_state = RUN; //machine state
static uint8_t major_state_transition = 1;

static waveType_t wave_type = WAVE_SQRE; //current generator wave type
uint16_t frequency = 100; //
uint16_t ADCread = 512;

/*--- Counters ---*/
volatile uint8_t t0_cnt = 0; //timer0 interrupt counter 
volatile uint16_t lut_pos = 0; //position in lookup table, used for sine gen

/*--- Flags ---*/
volatile uint8_t shown_status = 0;
volatile uint8_t cmd_recved = 0;

/*--- buffers ---*/
char cmd_buff[CMD_BUFF_LEN];
char * cmd_buff_pos=cmd_buff;

uint8_t last_len = 0;

/*--------- Main ---------*/
int main(void)
{
    /* set up pin directions */
    DDRB = 0xff;
    DDRC = 0xff;
    DDRD = 0xf0;
    
    /*configure timer 1 */
    TCNT1H = 0;
    TCNT1L = 0;

    TCCR1A = 0x00; //timer in normal mode,
    TCCR1B = 0x02; //presc = 8
    TIMSK1 = 0x02; //enable Interrupt for OC1A
    timer1_set_period_us(10000/frequency);
    //enable interrupts
    __asm__("sei;");
    
#if 1
    while(1) {
        ;
    }
#else
    //Configure pin interrupts
    EICRA = 0x00; //set both INT0 and INT1 as falling edge
    EIMSK = 0x03; //enable INT1 and INT0

    //initialize uart
    uart_init(BAUD_RATE);

    /* configure timer 0 */
    TCCR0A = 0x00;
    TCCR0B = 0x04; //presc = 256
    TIMSK0 = 0x01; //enable isr for timer 0 ovf 

    /* configure ADC  */
    ADMUX = 0b01000111; //Vref = pin AVCC (5V), ADC = pin ADC7
    ADCSRA = 0b10000110; //bit0: ADC enable, bits 2..0: prescaller
    ADCSRA |= (1 << ADSC); //starts first conversion
    uart_send_str("hello there!\r\n");
    set_bit(LED_ON);

    while(1) {
        //do state transition actions or run steady state code
        if(major_state_transition) {
            switch(major_state) {
            case STOP:
                timer1_stop();
                set_reg(DAC_HN, DAC_HN_MASK, 0x7f);
                set_reg(DAC_LN, DAC_LN_MASK, 0xff);
                rst_bit(LED_RUN);
                break;
            case RUN:
                timer1_start();
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
            major_state_transition = 0; //clear flag
        }
        else {
            switch(major_state) {
            case STOP:
                //wait
                break;
            case RUN:
#if 0 //TODO:Integrate ADC reading code into freq adjustment
                if (ADCSRA & (1 << ADIF)){ //if converion ended
                    ADCread = ADCW; //read conversion
                    ADCread = ((ADCread * 90)/1023); //0-1023 scale -> 0-90 scale
                    ADCSRA |= (1 << ADSC); //starts next conversion
				
                    //serial test print:
                    /*char buff[6];
                      snprintf(buff, 5, "%i", ADCread);
                      uart_send_str(buff);*/
                }
#endif
                break;
            }
        }
        //Show status line
        if (shown_status == 0) {
            show_status();
            shown_status = 1;
        }
        if(cmd_recved) {
            parse_cmd(cmd_buff); //parse incoming message
            cmd_recved = 0;
        }
    }
#endif
}

/*--------- Interrupts ---------*/
/**
   Update output waveform.
*/
ISR(TIMER1_COMPA_vect)
{
    volatile uint8_t v;

#if DEBUG_PULSE_PIN_ISR == 1
    set_bit(DEBG_PIN);
#endif

    set_2byte_reg(0x0000, TCNT1); //reset timer value
    /*
      read wave value from ROM (progam memory), and set port output
      (except for square wave).
    */
    #if USE_PROGMEM == 1
    switch(wave_type) {
    case WAVE_SINE:
        v = pgm_read_byte(sine_lut+lut_pos);
        break;
    case WAVE_TRGL:
        v = pgm_read_byte(trgl_lut+lut_pos);
        break;
    case WAVE_SWTT:
        v = pgm_read_byte(swtt_lut+lut_pos);
        break;
    case WAVE_SQRE:
        v = lut_pos < (LUT_LEN>>1) ? 0 : 255;
        break;
    default:
        v = 128;
        break;
    }
    #else
    switch(wave_type) {
    case WAVE_SINE:
        v = sine_lut[lut_pos];
        break;
    case WAVE_TRGL:
        v = trgl_lut[lut_pos];
        break;
    case WAVE_SWTT:
        v = swtt_lut[lut_pos];
        break;
    case WAVE_SQRE:
        v = lut_pos < (LUT_LEN>>1) ? 0 : 255;
        break;
    default:
        v = 128;
        break;
    }
    #endif
    DAC_HN = (DAC_HN & 0xf0) | (v >> 4);
    DAC_LN = (DAC_LN & 0xf0) | (v & 0x0f);
    lut_pos = lut_pos < LUT_LEN - 1 ? lut_pos + 1 : 0;

#if DEBUG_PULSE_PIN_ISR == 1
    rst_bit(DEBG_PIN);
#endif
}

/**
   used to periodically (~100 ms) show a status line on the uart. (only sets a
   flag to show later)
*/
ISR(TIMER0_OVF_vect)
{
    ++t0_cnt;
    if(t0_cnt >= 100) {
        t0_cnt = 0;
        shown_status = 0;
    }
}

/**
   start/stop button interrupt.
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
    *(cmd_buff_pos+1) = 0;
    //test for end of command
    if(*cmd_buff_pos == '\r') {
        //*(cmd_buff_pos+1) = 0; //null terminate string
        cmd_recved = 1;
    }
    ++cmd_buff_pos;
}

/*--------- Function definition ---------*/
/*--------- User interaction ---*/

/**
   Show running status line.
*/
void show_status(void)
{
    //delete previous text
    for(uint8_t i = last_len-1; i; --i) {
        //send ascii DEL
        uart_send_char(0x08);
    }
    const uint8_t bufflen = 150;
    char buff[bufflen];
    snprintf(buff, bufflen,
             "-----------\r\n"
             "status: %c wavef: %c freq: %03i\r\n"
             "cmd: %s\r\n"
             "-----------\r\n",
             major_state == RUN ? 'r' : 's', wave_type, frequency, cmd_buff);
    last_len = strnlen(buff,bufflen);
    uart_send_str(buff);
}

/**
   Parse received command
*/
void parse_cmd(char * _cmd_buff)
{
    const char help_str[] =

        "-------------------------------------------------------\r"
        "h - help\r"
        "r - run generator (plase configure first)\r"
        "s - stop generator\r"
        "c - configure generator - format: c <waveType> <freq>\r"
        "\t wavetypes:\r"
        "\t  - s - [s]ine\r"
        "\t  - q - s[q]uare\r"
        "\t  - w - sa[w]tooth\r"
        "\t  - t - [t]triangle\r"
        "\t frequency: 10-100 Hz, integer\r"
        "-------------------------------------------------------\r";

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
            f = f < 10 ? 10 : f;
            f = f - (f%10); //get closest power of ten
            wave_type = w;
            frequency = f;
            /*
              The timer frequency is 100 * f, because of 100 samples per cycle
            */
            timer1_set_period_us(1000/f);
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


/*--- Timer1 ---*/
void timer1_set_period_us(uint16_t t_us)
{
    const uint8_t tmr1_ofs = 4;
    const uint16_t maxt_us = 65535 >> 1; // divide by 2
    //test for greatest period that fits in OCreg
    t_us = t_us > (maxt_us) ? (maxt_us) :
        (t_us <= tmr1_ofs ? tmr1_ofs + 1 : t_us); 
    /**
       for a prescaler of 8 and clock of 16000000UL, every period is = 0.5 us
       so the compare value is 2*t_us
    */
    uint16_t OCval = t_us*2 - tmr1_ofs;
    //set_2byte_reg(OCval, OCR1A); //set output compare high and low byte
    OCR1AH = (OCval >> 8);
    OCR1AL = (OCval & 0xff);
}

/*------------- --------- DONE ---------- ---------------- */

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
