/// Syntacore SCR* infra
///
/// @copyright (C) Syntacore 2015-2017. All rights reserved.
/// @author mn-sc
///
/// @brief SCR* infra trap routines

#include <errno.h>
#include <stdint.h>

#include "platform_config.h"
#include "scbl.h"
#include "arch.h"
#include "uart.h"

__thread hart_storage hart_local_storage;

void print_bad_trap(const char *hdr_str, uintptr_t cause, uintptr_t* regs, uintptr_t cycle, uintptr_t instr)
{
    static const char* regnames[] = {
        "z  ", "ra ", "sp ", "gp ", "tp ", "t0 ", "t1 ", "t2 ",
        "s0 ", "s1 ", "a0 ", "a1 ", "a2 ", "a3 ", "a4 ", "a5 ",
        "a6 ", "a7 ", "s2 ", "s3 ", "s4 ", "s5 ", "s6 ", "s7 ",
        "s8 ", "s9 ", "s10", "s11", "t3 ", "t4 ", "t5 ", "t6 "
    };
    uart_putc('\n');
    uart_putc('\n');
    uart_puts(hdr_str);
    uart_puts(" trap ");
    uart_puthex(cause);
    uart_puts(" @ ");
    uart_puthex(read_csr(mepc));
    uart_puts(" (hart#");
    uart_putdec(arch_hartid());
    uart_puts(" status ");
    uart_puthex(read_csr(mstatus));
    uart_puts(" mtval ");
    uart_puthex(arch_badaddr());
    uart_puts(")\n   inst  ");
    uart_puthex(instr);
    uart_puts("    cycl  ");
    uart_puthex(cycle);
    uart_putc('\n');

    if (regs) {
        for (unsigned i = 0; i < 32; ++i) {
            uart_putc(' ');
            uart_putc(' ');
            uart_putc('0' + i / 10);
            uart_putc('0' + i % 10);
            uart_putc(' ');
            uart_puts(regnames[i]);
            uart_putc(' ');
            uart_puthex32(regs[i]);
            uart_putc(((i & 0x3) == 0x3) ? '\n' : ' ');
        }
    }
}

void __attribute__((noreturn)) bad_trap(void)
{
    print_bad_trap("Bad", read_csr(mcause), 0, read_csr(cycle), read_csr(instret));

    _hart_halt();
}

void __attribute__((noreturn))
bad_trap_handler(uintptr_t mcause, uintptr_t* regs, uintptr_t cycle, uintptr_t instr)
{
    print_bad_trap("+Unhandlable", mcause, regs, cycle, instr);

    _hart_halt();
}

static uintptr_t mcall_console_putchar(uint8_t ch)
{
    sc1f_uart_putchar(ch);
    return 0;
}

static int mcall_console_getchar(void)
{
    return sc1f_uart_getch_nowait();
}

static uintptr_t mcall_shutdown()
{
    extern void _start(void);
    _start();
    __builtin_unreachable();
}

uintptr_t mcall_trap(uintptr_t cause, uintptr_t* regs, uintptr_t cycle, uintptr_t instr)
{
    uintptr_t call_num = regs[17], arg0 = regs[10], retval;

    switch (call_num) {
    case SBI_CONSOLE_PUTCHAR:
        retval = mcall_console_putchar(arg0);
        break;
    case SBI_CONSOLE_GETCHAR:
        retval = mcall_console_getchar();
        break;
    case SBI_SHUTDOWN:
        retval = mcall_shutdown();
        break;
    default:
        retval = -ENOSYS;
        break;
    }
    regs[10] = retval;
    write_csr(mepc, read_csr(mepc) + 4);
    return 0;
}
