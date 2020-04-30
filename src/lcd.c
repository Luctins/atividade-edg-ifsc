/**
  -----------------------------------------------------------------------------
  @author Lucas Martins Mendes
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
   send a single command to the display.
*/
void lcd_cmd(unsigned char c, cmd_type_t cd)
{
  switch(cd)
    {
    case LCD_CMD:
      rst_bit(LCD_RS);
      break;
    case LCD_CHAR:
      set_bit(LCD_RS);
      break;
    default:
      return;
    }

  /*envia primeira metade do dado*/
#if USE_LOWER_NIBLE
  LCD_DATA = (LCD_DATA & 0xf0) | ( 0x0f & c);
#else
  LCD_DATA = (LCD_DATA & 0x0f) | ( 0xf0 & c);
#endif
  enable_pulse();

  /* Envia 2a metade do dado */
#if USE_LOWER_NIBLE
  LCD_DATA = (LCD_DATA & 0xf0) | ( 0x0f & (c << 4));
#else
  LCD_DATA = (LCD_DATA & 0x0f) | ( 0xf0 & (c << 4));
#endif

  //se for instrução de retorno ou limpeza espera LCD estar pronto
	if((cd==LCD_CMD) && (c<4))
    {
      _delay_ms(2);
    }
}

/**
   sequência ditada pelo fabricando do circuito integrado HD44780
 */
void lcd_4bit_init(void)
{
  rst_bit(LCD_RS); // inidica instrução
  rst_bit(LCD_EN); // enable em 0

  /**
     tempo para estabilizar a tensão do LCD, após VCC ultrapassar 4.5 V
     (na prática pode ser maior).
  */
	_delay_ms(20);


#if USE_LOWER_NIBLE
  LCD_DATA = (LCD_DATA & 0xf0) | 0x03;
#else
  LCD_DATA = (LCD_DATA & 0xf0) | 0x30;
#endif

  //habilitação respeitando os tempos de resposta do LCD
	enable_pulse();
	_delay_ms(5);
	enable_pulse();
	_delay_us(200);
	enable_pulse();

  /*
    até aqui ainda é uma interface de 8 bits. Muitos programadores desprezam os
    comandos acima, respeitando apenas o tempo de estabilização da tensão
    (geralmente funciona). Se o LCD não for inicializado primeiro no modo de
    8 bits, haverá problemas se o microcontrolador for inicializado e o display
    já o tiver sido.
  */

	//interface de 4 bits, deve ser enviado duas vezes (a outra está abaixo)
	#if USE_LOWER_NIBLE
		LCD_DATA = (LCD_DATA & 0xF0) | 0x02;
	#else
		LCD_DATA = (LCD_DATA & 0x0F) | 0x20;
	#endif

	enable_pulse();
  lcd_cmd(0x28,LCD_CMD); //interface de 4 bits 2 linhas (aqui se habilita as 2 linhas)
  lcd_cmd(0x08,LCD_CMD); //desliga o display
  lcd_cmd(0x01,LCD_CMD); //limpa todo o display
  lcd_cmd(0x0C,LCD_CMD); //mensagem aparente cursor inativo não piscando
  lcd_cmd(0x80,LCD_CMD); //inicializa cursor na primeira posição a esquerda - 1a linha
}

/**
   write a string to the display.
 */
void lcd_write(char *str)
{
   for (;*str;++str) lcd_cmd(*str,LCD_DATA);
}

/**
   Write to display flash
*/
void lcd_flash_write(const char * str)
{
  for (;pgm_read_byte(&(*str))!=0;++str)
    {
      lcd_cmd(pgm_read_byte(&(*str)),LCD_DATA);
    }
}
/*--------- END ---------*/
