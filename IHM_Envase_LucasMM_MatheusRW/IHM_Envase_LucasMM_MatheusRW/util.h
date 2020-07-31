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
  The name of Lucas Martins Mendes may not be used to endorse or
  promote products derived from this software without specific
  prior written permission.
  THIS SOFTWARE IS PROVIDED ''AS IS'' AND WITHOUT ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

  @author Lucas Martins Mendes
  @file   util.h
  @date   04/28/20
  -----------------------------------------------------------------------------
*/

#ifndef __UTIL_H__
#define __UTIL_H__


#define _E(x) x
#define E(e) _E(e)

#define USE_ASM_MACRO 0
/*--------- Bit test and set macros ---------*/

#if USE_ASM_MACRO

#define _set_bit(port, bit) __asm__ __volatile__ ("sbi %[p], %[b]":: [p] "I" _SFR_IO_ADDR(port), [b] "I" (bit))
#define _rst_bit(port, bit) __asm__ __volatile__ ("cbi %[p], %[b]":: [p] "I" _SFR_IO_ADDR(port), [b] "I" (bit))

#else

#define _set_bit(reg, bit) (reg) = ((1 << (bit)) | reg)
#define _rst_bit(reg, bit) (reg) =  (reg) & (0xff - (1 << (bit)))

#endif /* USE_ASM_MACRO */

#define _get_bit(port, bit) ((1 << (bit)) & port)
#define _cpl_bit(reg, bit)	(reg) ^= (1 << bit)


#define set_bit(P) _set_bit(P)
#define rst_bit(P) _rst_bit(P)
#define get_bit(P) _get_bit(P)
#define cpl_bit(p) _cpl_bit(P)

/*--------- Register Macros ---------*/

//#define set_mask(reg, mask, offset)	(reg |= (mask << offset))
//#define clr_mask(reg, mask, offset)	(reg &= ~(mask << offset)) /*!< '~' is Bitwise not */


#endif /* __UTIL_H__ */
