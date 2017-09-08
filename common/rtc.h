/// Syntacore SCR* infra
///
/// @copyright (C) Syntacore 2015-2017. All rights reserved.
/// @author mn-sc
///
/// @brief RTC defs and inline funcs

#ifndef SC_BL_RTC_H
#define SC_BL_RTC_H

#include <stdint.h>
#include <stdbool.h>
#include "platform_config.h"
#include "csr.h"

typedef unsigned long sys_tick_t;

#define SCR_RTC_CTL       (PLF_MTIMER_BASE+0)
#define SCR_RTC_DIVIDER   (PLF_MTIMER_BASE+4)
#define SCR_RTC_MTIME     (PLF_MTIMER_BASE+8)
#define SCR_RTC_MTIMEH    (PLF_MTIMER_BASE+12)
#define SCR_RTC_MTIMECMP  (PLF_MTIMER_BASE+16)
#define SCR_RTC_MTIMECMPH (PLF_MTIMER_BASE+20)
#define SCR_RTC_CTL_EN (1 << 0)
#define SCR_RTC_CTL_INTERNAL_SRC (0 << 1)
#define SCR_RTC_CTL_EXTERNAL_SRC (1 << 1)

#ifdef PLF_RTC_SRC_EXTERNAL
#if (PLF_RTC_TIMEBASE) > (PLF_RTC_SRC_EXTERNAL)
#error PLF_RTC_TIMEBASE > PLF_RTC_SRC_EXTERNAL
#endif
#define RTC_TIMEBASE_DIVISOR ((PLF_RTC_SRC_EXTERNAL) / (PLF_RTC_TIMEBASE) - 1)
#define SCR_RTC_CTL_SRC SCR_RTC_CTL_EXTERNAL_SRC
#else // PLF_RTC_SRC_EXTERNAL
#if (PLF_RTC_TIMEBASE) > (PLF_SYS_FREQ)
#error PLF_RTC_TIMEBASE > PLF_SYS_FREQ
#endif
#define RTC_TIMEBASE_DIVISOR ((PLF_SYS_FREQ) / (PLF_RTC_TIMEBASE) - 1)
#define SCR_RTC_CTL_SRC SCR_RTC_CTL_INTERNAL_SRC
#endif // PLF_RTC_SRC_EXTERNAL

static inline sys_tick_t now(void)
{
    return rdtime();
}

static inline long ticks2ms(sys_tick_t t)
{
    return t / (PLF_RTC_TIMEBASE / 1000);
}

static inline sys_tick_t ms2ticks(long t)
{
    return t * PLF_RTC_TIMEBASE / 1000;
}

static inline void rtc_delay_us(unsigned us)
{
    sys_tick_t t = now();
#if PLF_RTC_TIMEBASE != 1000000
    sys_tick_t ticks = us * (PLF_RTC_TIMEBASE / 976) / 1024;
#else
    sys_tick_t ticks = us;
#endif
    do ; while ((now() - t) < ticks);
}

static inline void scr_rtc_setcmp(uint64_t when)
{
#if __riscv_xlen == 32
    *(volatile uint32_t*)SCR_RTC_MTIMECMPH = 0xffffffff;
    *(volatile uint32_t*)SCR_RTC_MTIMECMP = (uint32_t)when;
    *(volatile uint32_t*)SCR_RTC_MTIMECMPH = (uint32_t)(when >> 32);
#else //  __riscv_xlen == 32
    *(volatile uint64_t*)SCR_RTC_MTIMECMP = when;
#endif //  __riscv_xlen == 32
}

static inline void scr_rtc_init(void)
{
    // configure RTC timebase (divisor) and reset time counters
    *(volatile uint32_t*)SCR_RTC_CTL = 0;
#if __riscv_xlen == 32
    *(volatile uint32_t*)SCR_RTC_MTIME = 0;
    *(volatile uint32_t*)SCR_RTC_MTIMEH = 0;
#else // __riscv_xlen == 32
    *(volatile uint64_t*)SCR_RTC_MTIME = 0;
#endif // __riscv_xlen == 32
#if __riscv_xlen == 32
    *(volatile uint32_t*)SCR_RTC_MTIMECMPH = 0xffffffff;
    *(volatile uint32_t*)SCR_RTC_MTIMECMP = 0xffffffff;
#else // __riscv_xlen == 32
    *(volatile uint64_t*)SCR_RTC_MTIMECMP = 0xffffffffffffffff;
#endif // __riscv_xlen == 32
    *(volatile uint32_t*)SCR_RTC_DIVIDER = RTC_TIMEBASE_DIVISOR;
    *(volatile uint32_t*)SCR_RTC_CTL = SCR_RTC_CTL_EN | SCR_RTC_CTL_SRC;
}

#endif // SC_BL_RTC_H
