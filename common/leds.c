/// Syntacore SCR* infra
///
/// @copyright (C) Syntacore 2015-2017. All rights reserved.
/// @author mn-sc
///
/// @brief LEDs defs and inline funcs

#include "leds.h"
#include "rtc.h"
#include "uart.h"

#ifdef PLF_HEXLED_ADDR

static const uint8_t HEX_DIGITS_TABLE[16] = {
    HEXLED_0,
    HEXLED_1,
    HEXLED_2,
    HEXLED_3,
    HEXLED_4,
    HEXLED_5,
    HEXLED_6,
    HEXLED_7,
    HEXLED_8,
    HEXLED_9,
    HEXLED_A,
    HEXLED_B,
    HEXLED_C,
    HEXLED_D,
    HEXLED_E,
    HEXLED_F
};

struct hex_seg_map {
    unsigned long addr;
    unsigned shift;
};

#ifdef PLF_HEXLED_ADDR_MAP
static const struct hex_seg_map HEX_MAP[] = {
    PLF_HEXLED_ADDR_MAP
};
#endif // PLF_HEXLED_ADDR_MAP

#if PLF_HEXLED_PORT_WIDTH == 4
typedef uint32_t pinled_port_mem;
#elif PLF_HEXLED_PORT_WIDTH == 2
typedef uint16_t pinled_port_mem;
#else
typedef uint8_t pinled_port_mem;
#endif


void sc1f_leds_hex(unsigned n, unsigned v)
{
#ifdef PLF_HEXLED_ADDR_MAP
    if (n < sizeof(HEX_MAP) / sizeof(*HEX_MAP)) {
        volatile pinled_port_mem *p = (volatile pinled_port_mem*)(HEX_MAP[n].addr);
        unsigned shift = HEX_MAP[n].shift;
        pinled_port_mem mask = ~(0xff << shift);
        *p = (*p & mask) | ((v ^ PLF_HEXLED_INV) << shift);
    }
#elif defined(PLF_HEXLED_ADDR) // PLF_HEXLED_ADDR_MAP
    volatile pinled_port_mem *p = (volatile pinled_port_mem*)PLF_HEXLED_ADDR;
    p[n] = v ^ PLF_HEXLED_INV;
#endif // PLF_HEXLED_ADDR_MAP
}

void sc1f_leds_hex_digit(unsigned n, unsigned v)
{
    sc1f_leds_hex(n, HEX_DIGITS_TABLE[v & 0xf]);
}
#endif // PLF_HEXLED_ADDR

#if defined(PLF_HEXLED_ADDR) || defined(PLF_PINLED_ADDR)
void sc1f_leds_init(void)
{
#ifdef PLF_HEXLED_ADDR_MAP
    for (unsigned i = 0; i < sizeof(HEX_MAP) / sizeof(*HEX_MAP); ++i)
        sc1f_leds_hex(i, 0);
#elif defined(PLF_HEXLED_ADDR) // PLF_HEXLED_ADDR_MAP
    for (unsigned i = 0; i < PLF_HEXLED_NUM; ++i)
        sc1f_leds_hex(i, 0);
#endif // PLF_HEXLED_ADDR_MAP
#ifdef PLF_PINLED_ADDR
    sc1f_leds_set(0);
#endif // PLF_PINLED_ADDR
}

void leds_print_info(void)
{
#ifdef PLF_HEXLED_ADDR
    uart_puthex(PLF_HEXLED_ADDR);
    uart_puts("\t ");
    uart_puts("\t" PLF_HEXLED_NAME "\n");
#endif // PLF_HEXLED_ADDR
#ifdef PLF_PINLED_ADDR
    uart_puthex(PLF_PINLED_ADDR);
    uart_puts("\t ");
    uart_puts("\t" PLF_PINLED_NAME "\n");
#endif // PLF_PINLED_ADDR
}
#endif // PLF_HEXLED_ADDR || PLF_PINLED_ADDR

#ifdef PLF_DIP_ADDR
void dips_print_info(void)
{
    uart_puthex(PLF_DIP_ADDR);
#ifdef PLF_DIP_IRQ
    uart_puts("\tirq ");
    uart_putdec(PLF_DIP_IRQ);
#else // PLF_DIP_IRQ
    uart_puts("\t ");
#endif // PLF_DIP_IRQ
    uart_puts("\t" PLF_DIP_NAME);
    uart_putc('\n');
}
#endif // PLF_DIP_ADDR
#ifdef PLF_BTN_ADDR
void btn_print_info(void)
{
    uart_puthex(PLF_BTN_ADDR);
#ifdef PLF_BTN_IRQ
    uart_puts("\tirq ");
    uart_putdec(PLF_BTN_IRQ);
#else // PLF_BTN_IRQ
    uart_puts("\t ");
#endif // PLF_BTN_IRQ
    uart_puts("\t" PLF_BTN_NAME);
    uart_putc('\n');
}
#endif //PLF_BTN_ADDR
