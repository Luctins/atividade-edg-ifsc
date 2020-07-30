/**
  -----------------------------------------------------------------------------
  @author Lucas Martins Mendes,  Matheus Reibnitz Willemann
  @file   LCD.h
  @date   04/30/20
  @brief  Adaptado do Material do professor Valdir Noll,
  IFSC Câmpus Florianópolis
  -----------------------------------------------------------------------------
*/

#ifndef __LCD_H__
#define __LCD_H__

#include <avr/io.h>
#define F_CPU  16000000UL
#include <util/delay.h>

/*--------- Constants ---------*/
#define USE_LOWER_NIBLE 1

/*--- Pin definition ---*/

#define LCD_PORT    	PORTD  /*!< LCD data port */
#define LCD_DDR       DDRD /*!< lcd DDR */

#if USE_LOWER_NIBLE == 1
#define LCD_DATA_MASK 0x0f   /*!< Mask of the used pins */
#else
#define LCD_DATA_MASK 0xf0   /*!< Mask of the used pins */
#endif

#define LCD_EN      PORTD,0 /*!< lcd enable pin*/
#define LCD_RS      PORTD,1  /*!< instruction/char pin (register select)*/
/*--- Macros ---*/

#define enable_pulse()                              \
	_delay_us(1); set_bit(LCD_EN);                    \
  _delay_us(1); rst_bit(LCD_EN);\
  _delay_us(45)

typedef enum cmd_type
  {
    LCD_CMD = 0,
    LCD_CHAR
  } cmd_type_t;
/*--------- Prototype dec ---------*/

void lcd_write(char *str);
void lcd_flash_write(const char * str);
void lcd_4bit_init(void);
void lcd_cmd(unsigned char c, cmd_type_t cd);

#define lcd_clear_display() lcd_cmd(0x01,LCD_CMD);


#endif /* __LCD_H__ */

/*--------- EOF ---------*/
