/// Syntacore SCR* infra
///
/// @copyright (C) Syntacore 2015-2017. All rights reserved.
/// @author mn-sc
///
/// @brief LEDs defs and inline funcs

#ifndef SC1_FPGA_LEDS_H
#define SC1_FPGA_LEDS_H

#include "platform_config.h"

/*
 *  Hex led codes:
 *   A --
 *  F |  | B
 *   G --
 *  E |  | C
 *   D -- o P
*/

// HEX symbols
#define HEXLED_0 (HEXLED_SEG_E | HEXLED_SEG_F | HEXLED_SEG_A | HEXLED_SEG_B | HEXLED_SEG_C | HEXLED_SEG_D)
#define HEXLED_1 (HEXLED_SEG_B | HEXLED_SEG_C)
#define HEXLED_2 (HEXLED_SEG_E | HEXLED_SEG_A | HEXLED_SEG_B | HEXLED_SEG_D | HEXLED_SEG_G)
#define HEXLED_3 (HEXLED_SEG_A | HEXLED_SEG_B | HEXLED_SEG_C | HEXLED_SEG_D | HEXLED_SEG_G)
#define HEXLED_4 (HEXLED_SEG_F | HEXLED_SEG_B | HEXLED_SEG_C | HEXLED_SEG_G)
#define HEXLED_5 (HEXLED_SEG_F | HEXLED_SEG_A | HEXLED_SEG_C | HEXLED_SEG_D | HEXLED_SEG_G)
#define HEXLED_6 (HEXLED_SEG_E | HEXLED_SEG_F | HEXLED_SEG_A | HEXLED_SEG_C | HEXLED_SEG_D | HEXLED_SEG_G)
#define HEXLED_7 (HEXLED_SEG_A | HEXLED_SEG_B | HEXLED_SEG_C)
#define HEXLED_8 (HEXLED_SEG_E | HEXLED_SEG_F | HEXLED_SEG_A | HEXLED_SEG_B | HEXLED_SEG_C | HEXLED_SEG_D | HEXLED_SEG_G)
#define HEXLED_9 (HEXLED_SEG_F | HEXLED_SEG_A | HEXLED_SEG_B | HEXLED_SEG_C | HEXLED_SEG_D | HEXLED_SEG_G)
#define HEXLED_A (HEXLED_SEG_E | HEXLED_SEG_F | HEXLED_SEG_A | HEXLED_SEG_B | HEXLED_SEG_C | HEXLED_SEG_G)
#define HEXLED_B (HEXLED_SEG_E | HEXLED_SEG_F | HEXLED_SEG_C | HEXLED_SEG_D | HEXLED_SEG_G)
#define HEXLED_C (HEXLED_SEG_E | HEXLED_SEG_F | HEXLED_SEG_A | HEXLED_SEG_D)
#define HEXLED_D (HEXLED_SEG_E | HEXLED_SEG_B | HEXLED_SEG_C | HEXLED_SEG_D | HEXLED_SEG_G)
#define HEXLED_E (HEXLED_SEG_E | HEXLED_SEG_F | HEXLED_SEG_A | HEXLED_SEG_D | HEXLED_SEG_G)
#define HEXLED_F (HEXLED_SEG_E | HEXLED_SEG_F | HEXLED_SEG_A | HEXLED_SEG_G)
#define HEXLED_DP (HEXLED_SEG_P)
#define HEXLED_EMPTY (0)

#ifndef __ASSEMBLER__

#include <stdint.h>

#ifdef PLF_PINLED_ADDR
static inline void sc1f_leds_set(unsigned long v)
{
    *(volatile unsigned long*)(PLF_PINLED_ADDR) = v ^ PLF_PINLED_INV;
}
#else // PLF_PINLED_ADDR
#define sc1f_leds_set(v) do {} while (0)
#endif // PLF_PINLED_ADDR

#ifdef PLF_HEXLED_ADDR
void sc1f_leds_hex(unsigned n, unsigned v);
void sc1f_leds_hex_digit(unsigned n, unsigned v);
#else // PLF_HEXLED_ADDR
#define sc1f_leds_hex(n, v) do {} while (0)
#define sc1f_leds_hex_digit(n, v) do {} while (0)
#endif // PLF_HEXLED_ADDR

#if defined(PLF_HEXLED_ADDR) || defined(PLF_PINLED_ADDR)
void sc1f_leds_init(void);
void leds_print_info(void);
#else // PLF_HEXLED_ADDR || PLF_PINLED_ADDR
#define sc1f_leds_init() do {} while (0)
#define leds_print_info() do {} while (0)
#endif // PLF_HEXLED_ADDR || PLF_PINLED_ADDR

#ifdef PLF_DIP_ADDR
void dips_print_info(void);
#else // PLF_DIP_ADDR
#define dips_print_info()  do {} while (0)
#endif // PLF_DIP_ADDR

#endif // __ASSEMBLER__

#endif // SC1_FPGA_LEDS_H
