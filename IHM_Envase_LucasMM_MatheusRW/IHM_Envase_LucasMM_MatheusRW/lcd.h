/**

   -----------------------------------------------------------------------------
   Copyright (c) 2020 Lucas Martins Mendes.
   All rights reserved.

   Redistribution and use in source and binary forms are permitted
   provided that the above copyright notice and this paragraph are
   duplicated in all such forms and that any documentation,
   advertising materials, and other materials related to such
   distribution and use acknowledge that the software was developed
   by Lucas Martins Mendes.
   The name of the Lucas Martins Mendes may not be used to endorse or
   promote products derived from this software without specific
   prior written permission.
   THIS SOFTWARE IS PROVIDED ''AS IS'' AND WITHOUT ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

   @author Lucas Martins Mendes, ,  Matheus Reibnitz Willemann
   @file   lcd.h
   @date   07/31/20
   @brief Adaptado do Material do professor Valdir Noll,
   IFSC Câmpus Florianópolis
   -----------------------------------------------------------------------------
*/

#ifndef __LCD_H__
#define __LCD_H__

/*--- Includes ---*/

#include <avr/io.h>

#define F_CPU  16000000UL
#include <util/delay.h>

/*--- Constants ---*/

#define USE_LOWER_NIBLE 0 //use PD4-7

/*--- Pin definition ---*/

#define LCD_PORT  PORTD  /*!< LCD data port */
#define LCD_DDR   DDRD   /*!< lcd DDR */

#if USE_LOWER_NIBLE == 1
#define LCD_DATA_MASK 0x0f   /*!< Mask of the used pins */
#else
#define LCD_DATA_MASK 0xf0   /*!< Mask of the used pins */
#endif

#define LCD_EN      PORTD,0 /*!< lcd enable pin*/
#define LCD_RS      PORTD,1  /*!< instruction/char pin (register select)*/

/*--- Macros ---*/

#define enable_pulse()                              \
    _delay_us(1); set_bit(LCD_EN);                  \
    _delay_us(1); rst_bit(LCD_EN);                  \
    _delay_us(45)

/*--------- Prototype dec ---------*/
typedef enum cmdType
{
    LCD_CMD = 0,
    LCD_CHAR
} cmdType_t;

void lcd_write(const char *str);
void lcd_4bit_init(void);
void lcd_cmd(unsigned char c, cmdType_t type);

inline void lcd_move_cursor(uint8_t c, uint8_t l)
{
    lcd_cmd(0x80 | ((l < 0x0f ? l : 0x0f )+(c > 0 ? 0x40 : 0)), LCD_CMD);
}

//void lcd_flash_write(const char * str);

#define lcd_clear() lcd_cmd(0x01,LCD_CMD);


#endif /* __LCD_H__ */

/*--------- EOF ---------*/
