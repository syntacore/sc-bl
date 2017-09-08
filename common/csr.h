/// Syntacore SCR* infra
///
/// @copyright (C) Syntacore 2015-2017. All rights reserved.
/// @author mn-sc
///
/// @brief Architecture specific CSR's defs and inlines

#ifndef SCR_INFRA_CSR_H
#define SCR_INFRA_CSR_H

#include <stdint.h>
#include <stdbool.h>
#include "platform_config.h"
#include "stringify.h"

#define _IMM_5BIT_CONST(val) \
    (__builtin_constant_p(val) && (unsigned long)(val) < 32)
#define _IMM_ZERO_CONST(val) \
    (__builtin_constant_p(val) && (val) == 0)

#define read_csr(reg)                           \
    ({                                          \
        unsigned long __tmp;                    \
        asm volatile ("csrr %0, " _TOSTR(reg)   \
                      : "=r"(__tmp));           \
        __tmp;                                  \
    })

#define write_csr(reg, val)                                 \
    do {                                                    \
        if (_IMM_ZERO_CONST(val)) {                         \
            asm volatile ("csrw " _TOSTR(reg) ", zero" ::); \
        } else if (_IMM_5BIT_CONST(val)) {                  \
            asm volatile ("csrw " _TOSTR(reg) ", %0"        \
                          :: "i"(val));                     \
        } else {                                            \
            asm volatile ("csrw " _TOSTR(reg) ", %0"        \
                          :: "r"(val));                     \
        }                                                   \
    } while (0)

#define swap_csr(reg, val)                                  \
    ({                                                      \
        unsigned long __tmp;                                \
        if (_IMM_ZERO_CONST(val)) {                         \
            asm volatile ("csrrw %0, " _TOSTR(reg) ", zero" \
                          :  "=r"(__tmp) :);                \
        } else if (_IMM_5BIT_CONST(val)) {                  \
            asm volatile ("csrrw %0, " _TOSTR(reg) ", %1"   \
                          : "=r"(__tmp) : "i"(val));        \
        } else {                                            \
            asm volatile ("csrrw %0, " _TOSTR(reg) ", %1"   \
                          : "=r"(__tmp) : "r"(val));        \
        }                                                   \
        __tmp;                                              \
    })

#define set_csr(reg, bit)                                   \
    ({                                                      \
        unsigned long __tmp;                                \
        if (_IMM_5BIT_CONST(bit)) {                         \
            asm volatile ("csrrs %0, " _TOSTR(reg) ", %1"   \
                          : "=r"(__tmp) : "i"(bit));        \
        } else {                                            \
            asm volatile ("csrrs %0, " _TOSTR(reg) ", %1"   \
                          : "=r"(__tmp) : "r"(bit));        \
        }                                                   \
        __tmp;                                              \
    })

#define clear_csr(reg, bit)                                 \
    ({                                                      \
        unsigned long __tmp;                                \
        if (_IMM_5BIT_CONST(bit)) {                         \
            asm volatile ("csrrc %0, " _TOSTR(reg) ", %1"   \
                          : "=r"(__tmp) : "i"(bit));        \
        } else {                                            \
            asm volatile ("csrrc %0, " _TOSTR(reg) ", %1"   \
                          : "=r"(__tmp) : "r"(bit));        \
        }                                                   \
        __tmp;                                              \
    })

#define rdtime() read_csr(time)
#define rdcycle() read_csr(cycle)
#define rdinstret() read_csr(instret)

#endif // SCR_INFRA_CSR_H
