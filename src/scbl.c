/// Syntacore SCR* infra
///
/// @copyright (C) Syntacore 2015-2017. All rights reserved.
/// @author mn-sc
///
/// @brief SCR* first stage boot loader

#include <stdint.h>
#include "scbl.h"
#include "arch.h"
#include "rtc.h"
#include "uart.h"
#include "leds.h"
#include "xmodem.h"
#include "stringify.h"

#define FW_VER "1.2"
#define FW_VER_CFG "scr1_RC"
#define COPYRIGHT_STR "Copyright (C) 2015-2019 Syntacore. All rights reserved."

#define MAX_XMODEM_RX_LEN PLF_MEM_SIZE

#define LEDS_TASK_DELAY_MS  200
#define MAX_HEX_LEDS        8

void dump_mem(uintptr_t addr, unsigned len)
{
    for (unsigned i = 0; len; ++i, addr += 16) {
        uart_puthex32(addr);
        uart_putc(':');
        for (unsigned j = 0; j < 16 && len > j; ++j) {
            uart_putc(' ');
            if (!(j & 3))
                uart_putc(' ');
            uart_puthex8(*(volatile uint8_t*)(addr + j));
        }
        uart_putc(' ');
        uart_putc('|');
        for (unsigned j = 0; j < 16 && len; ++j, --len) {
            unsigned ch = *(volatile uint8_t*)(addr + j);
            if (ch >= ' ' && ch < '\x7f')
                uart_putc(ch);
            else
                uart_putc('.');
        }
        uart_putc('|');
        uart_putc('\n');
    }
}

void hwinfo(void)
{
    const char *CPU_MXL[4] = {"??", "32", "64", "128"};
    const char *EXTENSIONS = "IEMAFDQLNCBTPVX";

    long cpu_id = (long)arch_isa();
    unsigned long mimpid = arch_impid();

    uart_puts("ISA: RV");
    unsigned mxl = 0;
    if (cpu_id < 0)
        mxl |= 0x2;
    if ((cpu_id << 1) < 0)
        mxl |= 0x1;
    uart_puts(CPU_MXL[mxl]);

    // decode cpu extensions
    for (const char *ep = EXTENSIONS; *ep; ++ep) {
        if (cpu_id & (1 << (*ep - 'A')))
            uart_putc(*ep);
    }

    uart_puts(" [");
    uart_puthex32(cpu_id);
    uart_puts("] IMPID: ");
    uart_puthex32(mimpid);
    uart_puts("\n");
    if (get_system_id()) {
        uart_puts("SYSID: ");
        uart_puthex32(get_system_id());
        uart_puts(" ");
    }
    if (get_build_id()) {
        uart_puts("BLDID: ");
        uart_puthex32(get_build_id());
    }
    uart_puts("\nPlatform: " _TOSTR(PLATFORM) ", cpuclk ");
    uart_putdec(PLF_CPU_FREQ / 1000000);
    if ((PLF_CPU_FREQ / 100000) % 10) {
        uart_puts(".");
        uart_putdec((PLF_CPU_FREQ / 100000) % 10);
    }
    uart_puts("MHz");
    uart_puts(", sysclk ");
    uart_putdec(PLF_SYS_FREQ / 1000000);
    if ((PLF_SYS_FREQ / 100000) % 10) {
        uart_putc('.');
        uart_putdec((PLF_SYS_FREQ / 100000) % 10);
    }
    uart_puts("MHz");
    uart_puts("\n");
#ifdef PLF_MEM_MAP
    uart_puts("Memory map:\n");
    for (unsigned i = 0; mem_regions[i].size > 0; ++i) {
        uart_puthex32(mem_regions[i].base);
        uart_putc('-');
        uart_puthex32(mem_regions[i].base + (mem_regions[i].size - 1));
        uart_putc('\t');
        uart_puthex32(mem_regions[i].attr);
        uart_putc('\t');
        uart_puts(mem_regions[i].name);
        uart_putc('\n');
    }
#endif // PLF_MEM_MAP
}

void show_header(void)
{
    uart_puts("\nSCR loader v" FW_VER "-" FW_VER_CFG);
    uart_puts("\n" COPYRIGHT_STR "\n");
}

static void cmd_plf_info(void *arg)
{
    uart_putc('\n');
    hwinfo();
    uart_puts("Platform configuration:\n");
    uart_print_info();
    leds_print_info();
    dips_print_info();
}

static void cmd_xload(void *arg)
{
    int st = xmodem_receive((uint8_t*)arg, MAX_XMODEM_RX_LEN);

    while (sc1f_uart_getch_nowait() >= 0);

    if (st < 0) {
        uart_puts("\nXmodem receive error: ");
        uart_putdec(-st);
        uart_putc('\n');
    }
    else  {
        uart_puts("\nXmodem successfully received ");
        uart_putdec(st);
        uart_puts(" bytes\n");
    }
}

static void cmd_start(void *arg)
{
    uart_putc('\n');
    sc1f_uart_tx_flush();
    rtc_delay_us(20000);
    asm volatile ("jalr %0" :: "r"(arg));
}

static void cmd_mem_dump(void *arg)
{
    dump_mem((uintptr_t)arg, 128);
}

static void cmd_mem_modify(void *arg)
{
    uart_puthex((unsigned long)arg);
    uart_putc(':');
    uart_putc(' ');
    unsigned long val = uart_read_hex();
    *(volatile unsigned long*)arg = val;
}

void usage(void);

static void cmd_show_commands(void *arg)
{
    usage();
}

#define SCBL_CMD_HIDDEN (1 << 8)
#define SCBL_CMD_ARG_ADDR (1 << 9)
#define SCBL_CMD_ARG_AUTOINC (1 << 10)

struct scbl_cmd {
    int key;
    const char *descr;
    void (*func)(void*);
    void *data;
};

static const struct scbl_cmd scbl_commands[] = {
    {'1' | SCBL_CMD_ARG_ADDR, "xmodem load @addr", cmd_xload, 0},
    {'g' | SCBL_CMD_ARG_ADDR, "start @addr", cmd_start, 0},
    {'d' | SCBL_CMD_ARG_ADDR | SCBL_CMD_ARG_AUTOINC, "dump mem", cmd_mem_dump, (void*)128},
    {'m' | SCBL_CMD_ARG_ADDR | SCBL_CMD_ARG_AUTOINC, "modify mem", cmd_mem_modify, (void*)4},
    {'i', "platform info", cmd_plf_info, 0},
    {' ' | SCBL_CMD_HIDDEN, 0, cmd_show_commands, 0},
};

void usage(void)
{
    uart_puts("\n");

    for (unsigned i = 0; i < sizeof(scbl_commands) / sizeof(*scbl_commands); ++i) {
        const struct scbl_cmd *cmd = &scbl_commands[i];
        if ((cmd->key & SCBL_CMD_HIDDEN) == 0 && cmd->descr) {
            uart_putc(cmd->key & 0xff);
            uart_puts(": ");
            uart_puts(cmd->descr);
            uart_puts("\n");
        }
    }
}

static void hex_leds_task(void)
{
#ifdef PLF_HEXLED_ADDR
    static uint32_t id_val, idx_i, idx_j;
    static sys_tick_t start_time;
    sys_tick_t t2;

    if (id_val == 0) {
        for (int i = 0; i < MAX_HEX_LEDS; i++) {
            sc1f_leds_hex(i, 0x0);
        }
        id_val = get_build_id();
        start_time = now();
        idx_i = 1;
        return;
    }

    t2 = now();
    if (t2 - start_time < ms2ticks(LEDS_TASK_DELAY_MS / 6)) {
        return;
    }
    start_time = now();

    if (idx_i <= MAX_HEX_LEDS) {
        if (idx_j < 9 - idx_i) {
            if (idx_j) {
                for (int i = 1; i < MAX_HEX_LEDS - idx_i; i++) {
                    sc1f_leds_hex(MAX_HEX_LEDS - 1 - i, 0x0);
                }
                sc1f_leds_hex_digit(MAX_HEX_LEDS - 1 - idx_j,
                    id_val >> 4 * (MAX_HEX_LEDS - idx_i));
            } else {
                sc1f_leds_hex_digit(MAX_HEX_LEDS - 1 - idx_j, 0xF);
            }
            idx_j++;
        } else {
            idx_j = 0;
            idx_i++;
        }
    }
#endif // PLF_HEXLED_ADDR
}

static void leds_task(void)
{
#ifdef PLF_PINLED_NUM
#if PLF_PINLED_NUM > 0
    static unsigned long leds_val;
    static sys_tick_t start_time;
    static int right;
    sys_tick_t t2;

    if (start_time == 0) {
        start_time = now();
        leds_val = 0x1;
    }

    t2 = now();
    if (t2 - start_time < ms2ticks(LEDS_TASK_DELAY_MS)) {
        return;
    }

    start_time = now();
    sc1f_leds_set(leds_val);

    if (right) {
        leds_val <<= 1;
    } else {
        leds_val >>= 1;
    }
    if (leds_val >= (1 << (PLF_PINLED_NUM - 1))) {
        right = 0;
    }
    if (leds_val <= 0x1) {
        right = 1;
        leds_val = 0x1;
    }
#endif // PLF_PINLED_NUM > 0
#endif // PLF_PINLED_NUM
}

void indication_tasks(void)
{
    hex_leds_task();
    leds_task();
}

int main(void)
{
    scr_rtc_init();
    sc1f_uart_init();
    sc1f_leds_init();

    show_header();
    hwinfo();
    usage();

    const struct scbl_cmd *prev_cmd = 0;
    void *prev_addr = 0;

    while (1) {
        uart_putc(':');
        uart_putc(' ');

        int c;
        while ((c = sc1f_uart_getch_nowait()) == -1) {
            indication_tasks();
        }
        sc1f_leds_set(0);

        const struct scbl_cmd *cmd = 0;

        for (unsigned i = 0; i < sizeof(scbl_commands) / sizeof(*scbl_commands); ++i) {
            if ((scbl_commands[i].key & 0xff) == c) {
                cmd = &scbl_commands[i];
                break;
            }
        }

        if (cmd) {
            uart_putc('\r');

            if (cmd->key & SCBL_CMD_ARG_ADDR) {
                uart_puts(cmd->descr);
                uart_puts("\naddr: ");
                prev_addr = (void*)uart_read_hex();
            } else {
                prev_addr = cmd->data;
            }
            cmd->func(prev_addr);
            prev_cmd = cmd;
        } else if (c == '\r') {
            if (prev_cmd && (prev_cmd->key & SCBL_CMD_ARG_AUTOINC)) {
                uart_putc('\r');
                prev_addr += (long)prev_cmd->data;
                prev_cmd->func(prev_addr);
            } else {
                uart_putc('\n');
            }
        } else {
            uart_putc(c);
            uart_puts(" - unknown command\n");
            cmd = 0;
        }
    }

    return 0;
}
