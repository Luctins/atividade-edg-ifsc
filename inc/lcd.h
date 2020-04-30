/**
  -----------------------------------------------------------------------------
  @author Lucas Martins Mendes
  @file   LCD.h
  @date   04/30/20
  @brief Adaptado do Material do professor Valdir Noll,
  IFSC Câmpus Florianópolis
  -----------------------------------------------------------------------------
*/

#ifndef __LCD_H__
#define __LCD_H__

/*--------- Constants ---------*/

/*--- Pin definition ---*/

#define LCD_DATA    	PORTD  /*!< LCD data port */
#define LCD_DATA_MASK 0xf0   /*!< Mask of the used pins */

#define LCD_EN      PORTB,1 /*!< pino de enable do LCD */
#define LCD_RS      PORTB,0 /*!< Pino de seleção instrução/char */
#define LCD_BKLIGHT PORTB,2

#define USE_LOWER_NIBLE 1

//#define tam_vetor	5	//número de digitos individuais para a conversão por ident_num()
//#define conv_ascii	48	//48 se ident_num() deve retornar um número no formato ASCII (0 para formato normal)

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
