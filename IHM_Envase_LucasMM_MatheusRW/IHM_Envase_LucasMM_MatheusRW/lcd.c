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

  /* wait for VCC to stabilize */
	_delay_ms(20);

  rst_bit(LCD_RS); // indica instrução
  rst_bit(LCD_EN); // enable em 0


  //habilitação respeitando os tempos de resposta do LCD
	enable_pulse();
	_delay_ms(5);
	enable_pulse();
	_delay_us(200);
	enable_pulse();

	#if USE_LOWER_NIBLE
		LCD_PORT |= 0x02;
	#else
		LCD_PORT |= 0x20;
	#endif

	enable_pulse();
  lcd_cmd(0x28); //interface de 4 bits 2 linhas (aqui se habilita as 2 linhas)
  lcd_cmd(0x08); //desliga o display
  lcd_cmd(0x01); //limpa todo o display
  lcd_cmd(0x0C); //mensagem aparente cursor inativo não piscando
  lcd_cmd(0x80); //inicializa cursor na primeira posição a esquerda - 1a linha
}

/**
   send a single command to the display.
*/
void lcd_cmd(unsigned char c /*!< command to send */)
{
    rst_bit(LCD_RS);
    /*send first nibble (high half) of data*/
#if USE_LOWER_NIBLE
    LCD_PORT |= (c & 0xf0) >> 4;
#else
    LCD_PORT |= (c & 0xf0);
#endif
    enable_pulse();

    /*send second (lower) nibble of data*/
#if USE_LOWER_NIBLE
    LCD_PORT |= (c & 0x0f);
#else
    LCD_PORT |= (c & 0x0f) << 4;
#endif
    enable_pulse();

    //se for instrução de retorno ou limpeza espera LCD estar pronto
    if(c<4)
    {
        _delay_ms(2);
    }
}

void lcd_send_char(const char c)
{
    set_bit(LCD_RS);

#if USE_LOWER_NIBLE
    LCD_PORT |= (c & 0xf0) >> 4;
#else
    LCD_PORT |= (c & 0xf0);
#endif
    enable_pulse();

    /*send second (lower) nibble of data*/
#if USE_LOWER_NIBLE
    LCD_PORT |= (c & 0x0f);
#else
    LCD_PORT |= (c & 0x0f) << 4;
#endif
    enable_pulse();

}
/**
   write a string to the display.
 */
void lcd_write(char *str)
{
   for (;*str;++str) lcd_send_char(*str);
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
