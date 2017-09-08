/// Syntacore SCR* infra
///
/// @copyright (C) Syntacore 2015-2017. All rights reserved.
/// @author mn-sc
///
/// @brief Architecture specific defs and inlines

#ifndef SCR_INFRA_ARCH_H
#define SCR_INFRA_ARCH_H

#include <stdint.h>
#include <stdbool.h>
#include "platform_config.h"
#include "csr.h"

static inline unsigned long __attribute__((const)) arch_isa(void)
{
  unsigned long res;
  asm ("csrr %0, misa" : "=r"(res));
  return res;
}

static inline unsigned long __attribute__((const)) arch_impid(void)
{
  unsigned long res;
  asm ("csrr %0, mimpid" : "=r"(res));
  return res;
}

static inline unsigned long __attribute__((const)) arch_hartid(void)
{
  unsigned long res;
  asm ("csrr %0, mhartid" : "=r"(res));
  return res;
}

static inline unsigned long __attribute__((const)) arch_badaddr(void)
{
  unsigned long res;
  asm ("csrr %0, 0x343" : "=r"(res));
  return res;
}

// arch specific CSRs

static inline uint32_t __attribute__((const)) get_system_id(void)
{
#ifdef PLF_SYS_ID_ADDR
    return *(volatile uint32_t*)PLF_SYS_ID_ADDR;
#else
    return 0;
#endif
}

static inline uint32_t __attribute__((const)) get_build_id(void)
{
#ifdef PLF_BLD_ID_ADDR
    return *(volatile uint32_t*)PLF_BLD_ID_ADDR;
#else
    return 0;
#endif
}

static inline void ifence(void)
{
    asm volatile ("fence.i" ::: "memory");
}

static inline void fence(void)
{
    asm volatile ("fence" ::: "memory");
}

static inline void wfi(void)
{
    asm volatile ("wfi" ::: "memory");
}

void __attribute__((noreturn)) _hart_halt(void);

#endif // SCR_INFRA_ARCH_H
