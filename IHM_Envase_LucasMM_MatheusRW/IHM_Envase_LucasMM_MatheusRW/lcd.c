/**
  -----------------------------------------------------------------------------
  @author Lucas Martins Mendes, Matheus Reibnitz Willemann
  @file   LCD.c
  @date   04/30/20

  @abstract
  adaptado do Material do professor Valdir Noll, IFSC Câmpus Florianópolis.

  AVR e Arduino: Técnicas de Projeto, 2a ed. - 2012.
  ------------------------------------------------------------------------------
  Sub-rotinas para o trabalho com um LCD 16x2 com via de dados de 4 bits
  Controlador HD44780	- Pino R/W aterrado
  A via de dados do LCD deve ser ligado aos 4 bits mais significativos ou
  aos 4 bits menos significativos do PORT do uC
  -----------------------------------------------------------------------------
*/

/*--------- Includes ---------*/

#define F_CPU  16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdint.h>


#include "util.h"

#include "lcd.h"


/*--------- Macros ---------*/

/*--------- Constants ---------*/

/*--------- Function dec ---------*/

/*--------- Globals ---------*/

/*--------- Function definition ---------*/

/**
   sequência ditada pelo fabricando do circuito integrado HD44780
*/
void lcd_4bit_init(void)
{

    /* reset lcd port bits and set RS low */
    //LCD_PORT &= 0x0f;
    rst_bit(LCD_RS);
    rst_bit(LCD_EN);

    /* wait for VCC to stabilize */
    _delay_ms(20);

    /**
       LCD startup sequence following the datasheet for 4 bits (page 46)
       @see https://www.sparkfun.com/datasheets/LCD/HD44780.pdf
    */
#if USE_LOWER_NIBLE == 1
    LCD_PORT = (LCD_PORT & 0xf0) | 0x03;
#else
    LCD_PORT = (LCD_PORT & 0x0f) | 0x30;
#endif
    enable_pulse();
    _delay_ms(5);
    enable_pulse();
    _delay_us(200);
    enable_pulse();

#if USE_LOWER_NIBLE
    LCD_PORT = (LCD_PORT & 0xf0) | 0x02;
#else
    LCD_PORT = (LCD_PORT & 0x0f) | 0x20;
#endif
    enable_pulse();


    /* set interface 4 bits, 2 lines, 8 dots font  */
    lcd_cmd(0b00101000,LCD_CMD);
    lcd_cmd(0x08,LCD_CMD); // turn off display
    lcd_cmd(0x01,LCD_CMD); // clear display
    lcd_cmd(0x0c,LCD_CMD); // turn displ. on, visible cursor, no blink
    lcd_cmd(0x80,LCD_CMD); //set CGRAM adress to 0 (1st position)
}

/**
   send a single command to the display.
*/
void lcd_cmd(unsigned char c, /*!< command to send */
             cmdType_t cmd /*!< command to send */)
{
    switch(cmd) {
    case LCD_CMD:
        rst_bit(LCD_RS);
        break;
    case LCD_CHAR:
        set_bit(LCD_RS);
        break;
    }

    /*send first nibble (high half) of data*/
#if USE_LOWER_NIBLE == 1
    LCD_PORT = (LCD_PORT & 0xf0) | ((c & 0xf0) >> 4);
#else
    LCD_PORT = (LCD_PORT & 0x0f) | (c & 0xf0);
#endif
    enable_pulse();

    /*send second (lower) nibble of data*/
#if USE_LOWER_NIBLE == 1
    LCD_PORT = (LCD_PORT & 0xf0) | (c & 0x0f);
#else
    LCD_PORT = (LCD_PORT & 0x0f) | ((c & 0x0f) << 4);
#endif
    enable_pulse();

    //wait if cmd is clear or return home (exec time ~1.52ms)
    if(c<4 && cmd == LCD_CMD)
    {
        _delay_ms(2);
    }
    //set_bit(LCD_RS);
    LCD_PORT &= ~(LCD_DATA_MASK);
}

/**
   write a string to the display.
 */
void lcd_write(const char * str)
{
    for (;*str;++str)
    {
        lcd_cmd(*str,LCD_CHAR);
    }
}

#if 0
/**
   Write to display flash
*/
void lcd_flash_write(const char * str)
{
  for (;pgm_read_byte(&(*str))!=0;++str)
    {
      lcd_cmd(pgm_read_byte(&(*str)),LCD_PORT);
    }
}
#endif
/*--------- END ---------*/
