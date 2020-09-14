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

#define F_CPU  8000000UL
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include <stdint.h>

#include <stdio.h>
#include <string.h>

#include "util.h"

/*--------- Macros ---------*/
#define DEBUG_PULSE_PIN_ISR 0
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
#define BAUD_RATE (38400)
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
    255, 252, 249, 247, 244, 242, 239, 237, 234, 232, 229, 226, 224, 221, 219, 216,
    214, 211, 209, 206, 204, 201, 198, 196, 193, 191, 188, 186, 183, 181, 178, 175,
    173, 170, 168, 165, 163, 160, 158, 155, 153, 150, 147, 145, 142, 140, 137, 135,
    132, 130, 127, 124, 122, 119, 117, 114, 112, 109, 107, 104, 102, 99, 96, 94,
    91, 89, 86, 84, 81, 79, 76, 73, 71, 68, 66, 63, 61, 58, 56, 53, 50, 48, 45, 43,
    40, 38, 35, 33, 30, 28, 25, 22, 20, 17, 15, 12, 10, 7, 5, 2
};

/*--- Pins ---*/
/*
  Since we'll be using the whole PORTB, the processor needs to use the internal
  oscillator so we can free the XTAL pins on PORTB.
 */
#define DAC_PORT PORTB

#define FREQ_ADJ_POT 7 /* DAC channel 7 */
#define DEBG_PIN PORTC,2

/*--- Buttons ---*/
#define BTN_SS_vect   INT1_vect
#define BTN_WAVE_vect INT0_vect
#define BTN_SS   PIND,3 // Start stop btn
#define BTN_WAVE PIND,2 // Toggle wave type button

#define LED_RUN  PORTC,1
#define LED_ON   PORTC,0
#define LED_SINE PORTD,7
#define LED_TRGL PORTD,6
#define LED_SQRE PORTD,5
#define LED_SWTT PORTD,4

/*--------- Predeclaration ---------*/

/*------ Types ------*/
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
    WAVE_SWTT = 'w', /*!< Sawtooth */
    WAVE_TRGL = 't'  /*!< Triangle */
} waveType_t;

/*------ Functions ------*/
/*------  Timer 1  ------*/
void timer1_set_period_us(uint16_t t_us);
inline void timer1_stop(void)  { TIMSK1 = 0x00; }
inline void timer1_start(void) { TIMSK1 = 0x02; }

/*------ UART ------*/
void uart_init(uint32_t baudrate);
void uart_send_char(const char c);
void uart_send_str(const char * buff);

/*------ Others ------*/
void parse_cmd(char * _cmd_buff);
uint8_t show_status(void);
void delete_text(uint8_t len);

/*--------- Globals ---------*/

/*------ States ------*/
static machineState_t major_state = STOP; // Machine state
static uint8_t major_state_transition = 1;

static waveType_t wave_type = WAVE_SQRE; // Current generator wave type
uint16_t frequency = 10;
uint16_t last_ADCread = 512;

/*------ Counters ------*/
volatile uint8_t t0_cnt = 0; // Timer0 interrupt counter
volatile uint16_t lut_pos = 0; // Position in lookup table, used for sine gen

/*------ Flags ------*/
volatile uint8_t shown_status = 0;
volatile uint8_t cmd_recved = 0;

/*------ Buffers ------*/
char cmd_buff[CMD_BUFF_LEN];
char * cmd_buff_pos=cmd_buff;

uint8_t last_status_len = 0;

/*--------- Main ---------*/
int main(void)
{
    /* Set up pin directions */
    DDRB = 0xff;
    DDRC = 0xff;
    DDRD = 0xf0;

    /* Configure timer 1 */
    TCCR1A = 0x00; // Timer in normal mode,
    TCCR1B = 0x02; // Presc = 8
    TIMSK1 = 0x02; // Enable Interrupt for OC1A
    timer1_set_period_us(10000/frequency);

    // Configure pin interrupts
    EICRA = 0x00; // Set both INT0 and INT1 as falling edge
    EIMSK = 0x03; // Enable INT1 and INT0

    // Initialize uart
    uart_init(BAUD_RATE);

    /* Configure timer 0 */
    TCCR0A = 0x00;
    TCCR0B = 0x04; // Presc = 256
    TIMSK0 = 0x01;

    __asm__("sei;"); // Enable interrupts

    /* Configure ADC  */
    ADMUX = 0b01000111; // Vref = pin AVCC (5V), ADC = pin ADC7
    ADCSRA = 0b10000110; // Bit0: ADC enable, bits 2..0: prescaller
    ADCSRA |= (1 << ADSC); // Starts first conversion


    uart_send_str("hello there!\r\n");
    set_bit(LED_ON);

    while(1) {
        // Do state transition actions or run steady state code
        if(major_state_transition) {
            switch(major_state) {
            case STOP:
                timer1_stop();
                DAC_PORT = 127; // Sets output to 0
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
            major_state_transition = 0; // Clear flag
        }
        else {
            switch(major_state) {
            case STOP:
                // Wait
                break;
            case RUN:
                // If conversion ended
                if (ADCSRA & (1 << ADIF)) {
                    uint32_t tmp = ADCW; // Read conversion
                    if (tmp != last_ADCread) { // If the value changed since last read
                        frequency = 10 + (((tmp * 90))/1023); // 0-1023 scale -> 10-100 scale
                        last_ADCread = tmp & 0xffff;
                        timer1_set_period_us(10000/frequency);
                    }
                    ADCSRA |= (1 << ADSC); // Starts next conversion
                }
                break;
            }
        }
        // Show status line
        if (shown_status == 0) {
            delete_text(last_status_len);
            last_status_len = show_status();
            shown_status = 1;
        }
        if(cmd_recved) {
            parse_cmd(cmd_buff); // Parse incoming message
            cmd_recved = 0;
        }
    }
}

/*--------- Interrupts ---------*/
/**
   Update output waveform.
   PLEASE DO NOT ALTER, as it alters the timing of the waveform generation
*/
ISR(TIMER1_COMPA_vect)
{
    volatile uint8_t v;

#if DEBUG_PULSE_PIN_ISR == 1
    set_bit(DEBG_PIN);
#endif

    set_2byte_reg(0x0000, TCNT1); // Reset timer value
    /*
      Read wave value from ROM (progam memory), and set port output
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
#else // Uses RAM
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

    DAC_PORT = v;

  // Increment LUT position ans tests if it should go back to 0
    lut_pos = lut_pos < LUT_LEN - 1 ? lut_pos + 1 : 0;

#if DEBUG_PULSE_PIN_ISR == 1
    rst_bit(DEBG_PIN);
#endif
}

/**
   Used to periodically (~100 ms) show a status line on the uart. (only sets a
   flag to show later
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
   Start/stop button interrupt.
*/
ISR(BTN_SS_vect)
{
    major_state = major_state == RUN ? STOP : RUN;
    major_state_transition = 1;
    //while(get_bit(BTN_SS)); // Wait button release;
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
   Handle serial data.
*/
ISR(USART_RX_vect) // Serial recieve
{
    *cmd_buff_pos = UDR0; // Save incoming char to buffer;

    // Test buffer boundary
    if (cmd_buff_pos + 1 >= (cmd_buff+CMD_BUFF_LEN)) {

        cmd_buff_pos = cmd_buff; // Discard input
        return;
    }
    *(cmd_buff_pos+1) = 0; // Null terminate string
    shown_status = 0; // Flag to refresh status
    // Test for end of command
    if(*cmd_buff_pos == '\r') {
        cmd_recved = 1;
    }
    ++cmd_buff_pos;
}

/*--------- Function definition  ---------*/
/*---------   User interaction   ---------*/
/**
  delete text.
*/
void delete_text(uint8_t len)
{
    // Delete previous text
    for(uint8_t i = len; i; --i) {
        uart_send_char(0x08); // Send Backspace
    }
}

/**
   Show running status line.
*/
uint8_t show_status(void)
{
    const uint8_t bufflen = 150;
    char buff[bufflen];
    snprintf(buff, bufflen,
             "---------------------------------\r"
             "status: %c wavef: %c freq: %03iHz\r"
             "cmd: %s\r"
             "---------------------------------\r",
             major_state == RUN ? 'r' : 's', wave_type, frequency, cmd_buff);
    uart_send_str(buff);
    return strnlen(buff, bufflen);
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
        "\t  - t - [t]riangle\r"
        "\t frequency: 10-100 Hz, integer\r"
        "-------------------------------------------------------\r";

    cmd_t cmd = _cmd_buff[0];
    cmd_buff_pos = cmd_buff;
    char w = 0;
    uint16_t f = 9999;
    switch(cmd) {
    case CMD_RUN:
        major_state = RUN;
        major_state_transition = 1;
        break;
    case CMD_CFG:
      // Reads text input to variables w & f
        sscanf(_cmd_buff, "%*c %c %u\n", &w, &f);
        if(w && (f <= 100) ) {
            f = f < 10 ? 10 : f; // Sets f to 10 if f < 10, leaves otherwise
            //f = f - (f%10); // Get closest power of ten
            wave_type = w;
            frequency = f;
            /*
              The timer frequency is 100 (Hz * 100 samples/sec) / f
            */
            timer1_set_period_us(10000/f);
            serial_debug("ok");
        } else {
            serial_debug("invalid arg");
        }
        break;
    case CMD_STOP:
        major_state = STOP;
        major_state_transition = 1;
        serial_debug("stopped");
        break;
    default:
        serial_debug("invalid cmd");
    case CMD_HLP:
        uart_send_str(help_str);
        last_status_len = 0; //No status line to delete
        break;
    }
    *cmd_buff_pos = 0; // Reset cmd buffer
}


/*------ Timer1 ------*/
void timer1_set_period_us(uint16_t t_us)
{
    const uint8_t tmr1_ofs = 5; // Adjusted by hand from the ISR exec time
    const uint16_t maxt_us = 0xffff; // Divide by 2
    // Test for greatest period that fits in OCreg
    t_us = t_us > (maxt_us) ? (maxt_us) :
        (t_us <= tmr1_ofs ? tmr1_ofs + 1 : t_us);
    /**
       For a prescaler of 8 and clock of 16000000UL, every period is = 0.5 us
       so the compare value is 2*t_us
    */
    uint16_t OCval = (t_us) - tmr1_ofs;
    set_2byte_reg(OCval, OCR1A); // Set output compare high and low byte
}

/*--------- UART ---------*/
void uart_send_str(const char * buff)
{
    // Increment pointer until found null byte
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
    UCSR0B = (1 << TXEN0)|(1 << RXEN0); // Enable serial RX and TX
    UCSR0B |= (1 << RXCIE0); // Enable serial reception on interruption
    UCSR0C = (1 << UCSZ00) | (1 << UCSZ01); // 8bits per frame
    UBRR0H = (((F_CPU / ((uint32_t)16 * baudrate) ) - 1) >> 8);
    UBRR0L = ((F_CPU / ((uint32_t)16 * baudrate) ) - 1); // Modo Normal Assíncrono;
}

/*--------- EOF ---------*/
