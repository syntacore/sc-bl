/*
 * Copyright (C) 2015, Syntacore Ltd.
 * All Rights Reserved.
 */

/// Syntacore SCR* infra
///
/// @copyright Copyright (C) 2015-2025, Syntacore Ltd. All rights reserved.
/// @author mn-sc
///
/// @brief SCR loader

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "vm.h"
#include "hls.h"
#include "arch.h"
#include "cache.h"
#include "rtc.h"
#include "mmu.h"
#if PLF_PMP_SUPPORT
#include "pmp.h"
#endif // PLF_PMP_SUPPORT
#include "uart.h"
#include "xmodem.h"
#include "kbd.h"
#include "xqspi.h"
#include "mrt.h"
#include "stringify.h"
#include "reg_detect.h"
#include "clk.h"

#include <hal/drivers/leds.h>

#define FW_VER "1.3"
#ifndef FW_VER_CFG
#define FW_VER_CFG "custom"
#endif // FW_VER_CFG
#define COPYRIGHT_STR "Copyright (C) 2015-2025 Syntacore, Syntacore Ltd. All rights reserved."
#ifdef VERSION_TAG
#define FW_VER_TAG __xstringify(VERSION_TAG)
#else
#define FW_VER_TAG __DATE__ " " __TIME__
#endif

// xmodem max size 1G
#define MAX_XMODEM_RX_LEN (PLF_MEM_SIZE > 1*1024*1024*1024UL ? 1*1024*1024*1024 : (unsigned)PLF_MEM_SIZE)

bool enable_dbg_out =
#ifdef PLF_NON_INTERACTIVE
#if PLF_RUN_QUIET_MODE
    0
#else
    1
#endif // PLF_RUN_QUIET_MODE
#else
    0
#endif // PLF_NON_INTERACTIVE
    ;

void dump_mem(uintptr_t addr, unsigned len)
{
    for (; len; addr += 16) {
        uart_puthex(addr);
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
#if PLF_MEM_DUMP32
void dump_mem32(uintptr_t addr, unsigned len)
{
    for (unsigned i = 0; len; ++i, addr += 16) {
        uart_puthex(addr);
        uart_putc(':');
        for (unsigned j = 0; j < 16 && len > j; j += 4) {
            uart_putc(' ');
            uint32_t v = (*(volatile uint32_t*)(addr + j));
            uart_puthex8((v >> 0) & 0xFF);
            uart_puthex8((v >> 8) & 0xFF);
            uart_puthex8((v >> 16) & 0xFF);
            uart_puthex8((v >> 24) & 0xFF);
        }
        uart_putc(' ');
        uart_putc('|');
        for (unsigned j = 0; j < 16 && (len >= 4); j += 4, len -= 4) {
            uint32_t v = (*(volatile uint32_t*)(addr + j));
            unsigned ch;
            ch = (v >> 0) & 0xFF;
            if (ch >= ' ' && ch < '\x7f') uart_putc(ch);
            else uart_putc('.');

            ch = (v >> 8) & 0xFF;
            if (ch >= ' ' && ch < '\x7f') uart_putc(ch);
            else uart_putc('.');

            ch = (v >> 16) & 0xFF;
            if (ch >= ' ' && ch < '\x7f') uart_putc(ch);
            else uart_putc('.');

            ch = (v >> 24) & 0xFF;
            if (ch >= ' ' && ch < '\x7f') uart_putc(ch);
            else uart_putc('.');
        }
        uart_putc('|');
        uart_putc('\n');
    }
}
#endif 

static void dump_clk_mhz(unsigned long clk, int units)
{
    uart_putdec(clk / 1000000);
    if ((clk / 100000) % 10) {
        uart_puts(".");
        uart_putdec((clk / 100000) % 10);
    }

    if (units)
        uart_puts("MHz");
}

#ifdef PLF_CACHE_CFG
static void cache_info(void)
{
    bool l1i_enabled = read_csr(SCR_CSR_CACHE_GLBL) & CACHE_GLBL_L1I_EN;
    bool l1d_enabled = read_csr(SCR_CSR_CACHE_GLBL) & CACHE_GLBL_L1D_EN;
    bool l1_identical = scr_l1caches_identical();
    bool l2_identical = scr_l2caches_identical();

    if (!l1_identical)
        printf("Different per-hart L1 configurations detected\n");
    if (!l2_identical)
        printf("Different per-hart L2 configurations detected\n");

    if (!l1_identical || !l2_identical) {
        for (unsigned hartn = 0; hartn < num_harts; ++hartn) {
            printf("[hart %u]: \t", hartn);
            show_l1cache_info("L1i", hart_hls(hartn)->l1i_info, l1i_enabled);
            printf("           \t");
            show_l1cache_info("L1d", hart_hls(hartn)->l1d_info, l1d_enabled);
            if (!l2_identical) {
                printf("           \t");
                show_l2cache_info(hartn);
            }
        }
    } else {
        show_l1cache_info("L1i", HLS()->l1i_info, l1i_enabled);
        show_l1cache_info("L1d", HLS()->l1d_info, l1d_enabled);
    }
    if (l2_identical)
        show_l2cache_info(/* hartn = */ 0);
    show_l3cache_info();
}

static void cmd_flip_l1cache(void *arg)
{
    if (cache_l1_enabled())
        cache_l1_disable();
    else
        cache_l1_enable();

    cache_info();
}

#if PLF_L2CTL_BASE
static void cmd_flip_l2cache(void *arg)
{
    if (scr_l2cache_is_enabled())
        scr_l2cache_disable();
    else
        scr_l2cache_enable();

    cache_info();
}
#endif // PLF_L2CTL_BASE
#endif // PLF_CACHE_CFG

// TODO: rv32 implementation
#if (__riscv_xlen > 32) && defined(PLF_MTIMER_BASE)
static bool scr_rtc_check(bool dbg_print_details)
{
    int res = 1;
#ifndef PLF_RUN_QUIET_MODE
    printf("RTC Check%s", (dbg_print_details ? "\n" : ":"));
#endif // PLF_RUN_QUIET_MODE
#if __riscv_xlen > 32
    unsigned long plf_cpu_clk = clk_masterhart_get();

    uint32_t mtc0 = *(volatile uint32_t*)SCR_RTC_CTL;
    *(volatile uint32_t*)SCR_RTC_CTL = 0;
    uint32_t mtc1 = *(volatile uint32_t*)SCR_RTC_CTL;
    uint32_t mtd0 = *(volatile uint32_t*)SCR_RTC_DIVIDER;
    *(volatile uint32_t*)SCR_RTC_DIVIDER = 0x55;
    uint32_t mtd1 = *(volatile uint32_t*)SCR_RTC_DIVIDER;
    *(volatile uint32_t*)SCR_RTC_DIVIDER = mtd0;
    uint32_t mtd2 = *(volatile uint32_t*)SCR_RTC_DIVIDER;

    if (dbg_print_details) {
        printf("initial divider = %x\n", (unsigned)mtd0);
        printf("write: 55, read: %x\n", (unsigned)mtd1);
        printf("write: %x, read: %x\n", mtd0, (unsigned)mtd2);
    }

    if (mtd1 != 0x55 || mtd2 != mtd0) {
        res = 0;
        if (dbg_print_details) {
            printf("FAIL: divider access check\n");
        }
    }

	uint32_t rtc_ctl = SCR_RTC_CTL_EN;
	if(clk_mtimer_get().clock_source == SCR_CLK_MTIMER_EXTERNAL) {
		rtc_ctl |= SCR_RTC_CTL_EXTERNAL_SRC;
	} else {
		rtc_ctl |= SCR_RTC_CTL_INTERNAL_SRC;
	}
	*(volatile uint32_t*)SCR_RTC_CTL = rtc_ctl;

    uint32_t mtc2 = *(volatile uint32_t*)SCR_RTC_CTL;

    if (dbg_print_details) {
        printf("initial ctrl = %x\n", (unsigned)mtc0);
        printf("write: 0, read: %x\n", (unsigned)mtc1);
        printf("write: %x, read: %x\n", rtc_ctl, (unsigned)mtc2);
    }

    if (mtc1 != 0 || mtc2 != rtc_ctl) {
        res = 0;
        if (dbg_print_details) {
            printf("FAIL: mtimer control access check\n");
        }
    }

    long mtt0 = *(volatile uint64_t*)SCR_RTC_MTIME;
    *(volatile uint64_t*)SCR_RTC_MTIME = 0x1020304050607080;
    long mtt1 = *(volatile uint64_t*)SCR_RTC_MTIME;
    *(volatile uint64_t*)SCR_RTC_MTIME = 0x7263544536271809;
    long mtt2 = *(volatile uint64_t*)SCR_RTC_MTIME;
    *(volatile uint64_t*)SCR_RTC_MTIME = 0x1030700000000000;
    long mtt3 = *(volatile uint64_t*)SCR_RTC_MTIME;
    *(volatile uint64_t*)SCR_RTC_MTIME = mtt0 & ~7;
    long mtt4 = *(volatile uint64_t*)SCR_RTC_MTIME;

    if (dbg_print_details) {
        printf("initial mtime = %lx\n", mtt0);
        printf("write: 1020304050607080, read: %lx\n", mtt1);
        printf("write: 7263544536271809, read: %lx\n", mtt2);
        printf("write: 1030700000000000, read: %lx\n", mtt3);
        printf("write: %lx, read: %lx\n", mtt0 & ~7, mtt4);
    }

    if ((mtt1 & ~7) != 0x1020304050607080
        || (mtt2 & ~7) != 0x7263544536271808
        || (mtt3 & ~7) != 0x1030700000000000
        || (mtt4 & ~7) != (mtt0 & ~7)) {
        res = 0;
        if (dbg_print_details) {
            printf("FAIL: mtime access check\n");
        }
    }

    long mttc0 = *(volatile uint64_t*)SCR_RTC_MTIMECMP;
    *(volatile uint64_t*)SCR_RTC_MTIMECMP = 0x7820304050607080;
    long mttc1 = *(volatile uint64_t*)SCR_RTC_MTIMECMP;
    *(volatile uint64_t*)SCR_RTC_MTIMECMP = 0x7363544536271809;
    long mttc2 = *(volatile uint64_t*)SCR_RTC_MTIMECMP;
    *(volatile uint64_t*)SCR_RTC_MTIMECMP = UINT64_MAX;
    long mttc3 = *(volatile uint64_t*)SCR_RTC_MTIMECMP;

    if (dbg_print_details) {
        printf("initial mtimecmp = %lx\n", mttc0);
        printf("write: 7820304050607080, read: %lx\n", mttc1);
        printf("write: 7363544536271809, read: %lx\n", mttc2);
        printf("write: FFFFFFFFFFFFFFFF, read: %lx\n", mttc3);
    }

    if (mttc1 != 0x7820304050607080
        || mttc2 != 0x7363544536271809
        || mttc3 != UINT64_MAX) {
        res = 0;
        if (dbg_print_details) {
            printf("FAIL: mtimecmp access check\n");
        }
    }

    long mt = *(volatile uint64_t*)SCR_RTC_MTIME;
    long t = rdtime();
    long dt = mt - t;
    long mt2, t2, dt2;

    if (dbg_print_details) {
        printf("mtime: %ld, rdtime: %ld, diff: %ld\n", mt, t, dt);
    }

    if (dt > 5 || dt < -5) {
        res = 0;
        if (dbg_print_details) {
            printf("Time mismatch: mtime vs rdtime\n");
            printf("FAIL: mtime check\n");
        }
    }

    if (dbg_print_details) {
        printf("Wait for tick...\n");
    }

    mt2 = *(volatile uint64_t*)SCR_RTC_MTIME;
    t2 = rdtime();
    for (int i = 0; i < (plf_cpu_clk / PLF_RTC_TIMEBASE) * 100; ++i) {
        mt2 = *(volatile uint64_t*)SCR_RTC_MTIME;
        t2 = rdtime();

        if (mt2 != mt && t2 != t)
            break;
    }

    dt2 = mt2 - t2;

    if (dbg_print_details) {
        printf("mtime: %ld, rdtime: %ld, diff: %ld\n", mt2, t2, dt2);
    }

    if (t2 == t || mt2 == mt || (dt2 > dt + 1) || (dt > dt2 + 1)) {
        res = 0;
        if (dbg_print_details)
            printf("FAIL: timer check\n");
    }
#else
    /* uint32_t mt32_lo = *(volatile uint32_t*)SCR_RTC_MTIME; */
    /* uint32_t mt32_hi = *(volatile uint32_t*)SCR_RTC_MTIMEH; */
    /* uint32_t t_lo = rdtime(); */
    /* uint32_t t_hi = rdtimeh(); */
#endif

    printf("%s %s\n", (dbg_print_details ? "RTC Check:" : ""), (res ? "OK" : "FAIL"));

    if (dbg_print_details)
        printf("\n");

    return res;
}
#else // PLF_MTIMER_BASE
#define scr_rtc_check(dbg_print_details) do {} while (0)
#endif // PLF_MTIMER_BASE

void print_scr_rev(void)
{
    unsigned long rev;

    // try to use marchid for core detection

    rev = arch_coreid();

    if (rev > 0) {
        uart_puts("Running on SCR");
        uart_putdec(rev);
        uart_puts("\n");

        return;
    }

    // Unknown core
    uart_puts("Running on unknown core\n");
}

void hwinfo(void)
{
    const char *CPU_MXL[4] = {"??", "32", "64", "128"};
    const char *EXTENSIONS = "IEMAFDQLNCBKTPVXHSU";

    long cpu_id = (long)arch_cpuid();
    unsigned long mvendord = arch_vendorid();
    unsigned long mimpid = arch_impid();
    unsigned long marchid = arch_archid();

    uart_puts("ISA: RV");
    unsigned mxl = 0;
    if (cpu_id < 0)
        mxl |= 0x2;
    if ((cpu_id << 1) < 0)
        mxl |= 0x1;
    uart_puts(CPU_MXL[mxl]);

    // decode cpu extensions
    for (const char *ep = EXTENSIONS; *ep; ++ep) {
        if (cpu_id & (1L << (*ep - 'A')))
            uart_putc(*ep);
    }

    uart_puts(" [");
    uart_puthex(cpu_id);
    uart_puts("]\n");

    uart_puts("IMPID: ");
    uart_puthex(mimpid);
    uart_puts("\n");
    uart_puts("ARCHID: ");
    uart_puthex(marchid);
    uart_puts("\n");
    uart_puts("VENDORID: ");
    uart_puthex(mvendord);
    uart_puts("\n");

    if (get_system_id()) {
        uart_puts("SYSID: ");
        uart_puthex32(get_system_id());
        uart_puts(" ");
    }
    if (clk_buildid_get()) {
        uint32_t id = clk_buildid_get();
        uart_puts("BLDID: ");
        uart_puthex32(id);
        leds_hex_digit(7, id);
        id >>= 4;
        leds_hex_digit(6, id);
        id >>= 4;
        leds_hex_digit(5, id);
        id >>= 4;
        leds_hex_digit(4, id);
        id >>= 4;
        leds_hex_digit(3, id);
        id >>= 4;
        leds_hex_digit(2, id);
        id >>= 4;
        leds_hex_digit(1, id);
        id >>= 4;
        leds_hex_digit(0, id);
    }
    uart_puts("\nPlatform: " __xstringify(PLATFORM));

    uart_puts(", cpuclk ");

    unsigned long plf_cpu_clk = clk_masterhart_get();
    dump_clk_mhz(plf_cpu_clk, true);

    uart_puts(", sysclk ");
    dump_clk_mhz(clk_sys_get(), true);

    unsigned long plf_rtc_clk = clk_mtimer_get().clk;

    uart_puts(", rtcclk ");
    dump_clk_mhz(plf_rtc_clk, true);
    uart_puts("\n");

    if (PLF_RTC_TIMEBASE > plf_rtc_clk) {
        printf("RTC is too slow for requested settings: %lu > %lu\n",
               (unsigned long)PLF_RTC_TIMEBASE, plf_rtc_clk);
    }

    print_scr_rev();
#if PLF_RVA22_SUPPORT
    uart_puts("RVA22 enabled\n");
#endif
#if PLF_DMA_COHERENT
    uart_puts("DMA coherent\n");
#endif

    if (enable_dbg_out) {
#ifdef PLF_CACHE_CFG
        cache_info();
#endif // PLF_CACHE_CFG
#ifdef PLF_MEM_MAP
        uart_puts("Memory map:\n");
        for (unsigned i = 0; i < mem_region_count; ++i) {
            unsigned long sz = mem_regions[i].size;
            uart_puthex(mem_regions[i].base);
            /* uart_putc('-'); */
            /* uart_puthex(mem_regions[i].base + (sz - 1)); */
            uart_putc(' ');
            uart_putc(' ');
            if (sz >= (1 << 30) && ((sz & ((1 << 30) - 1)) == 0)) {
                uart_putdec(sz >> 30);
                uart_putc('G');
            } else if (sz >= (1 << 20) && ((sz & ((1 << 20) - 1)) == 0)) {
                uart_putdec(sz >> 20);
                uart_putc('M');
            } else if (sz >= (1 << 10) && ((sz & ((1 << 10) - 1)) == 0)) {
                uart_putdec(sz >> 10);
                uart_putc('K');
            } else {
                uart_putdec(sz);
            }
            uart_putc('\t');
#if PLF_MPU_SUPPORT
            uart_puthex32(mem_regions[i].attr);
#endif // PLF_MPU_SUPPORT
            uart_putc(' ');
            uart_putc(' ');
            uart_puts(mem_regions[i].name);
            uart_putc('\n');
        }
#endif // PLF_MEM_MAP
#if PLF_MPU_SUPPORT
        write_csr(SCR_CSR_MPU_SEL, ~0);
        unsigned long rmax = read_csr(SCR_CSR_MPU_SEL);
        uart_puts("MPU regions (");
        uart_putdec(rmax + 1);
        uart_puts("):\n");
        for (unsigned long rn = 0; rn <= rmax; ++rn) {
            write_csr(SCR_CSR_MPU_SEL, rn);
            unsigned long ctrl = read_csr(SCR_CSR_MPU_CTRL);
            if (ctrl & SCR_MPU_CTRL_VALID) {
                unsigned long base =  read_csr(SCR_CSR_MPU_ADDR);
                unsigned long mask =  read_csr(SCR_CSR_MPU_MASK);
                uart_puthex8(rn);
                uart_putc(' ');
                uart_puthex32(ctrl);
                uart_putc(' ');
                uart_puthex(base << 2);
                uart_putc(' ');
                uart_puthex(mask << 2);
                uart_putc('\n');
            }
        }
#endif // PLF_MPU_SUPPORT
#if PLF_PMP_SUPPORT
        const int rmax_pmp = PMP_ENTRIES;
        uart_puts("PMP regions (");
        uart_putdec(rmax_pmp);
        uart_puts("):\n");
        uint64_t pmp_addr_reg[PMP_ENTRIES];
#define PMPADDR_READ(x) pmp_addr_reg[x] = read_csr(CSR_PMPADDR##x);
        PMPADDR_READ(0);
        PMPADDR_READ(1);
        PMPADDR_READ(2);
        PMPADDR_READ(3);
        PMPADDR_READ(4);
        PMPADDR_READ(5);
        PMPADDR_READ(6);
        PMPADDR_READ(7);
        PMPADDR_READ(8);
        PMPADDR_READ(9);
        PMPADDR_READ(10);
        PMPADDR_READ(11);
        PMPADDR_READ(12);
        PMPADDR_READ(13);
        PMPADDR_READ(14);
        PMPADDR_READ(15);
#undef PMPADDR_READ
        for (unsigned long rn = 0; rn < rmax_pmp; ++rn) {
            uint8_t ctrl = read_pmp_ctrl(rn);
            if (PMP_NAPOT == (ctrl & PMP_A)) {
                uint64_t region = pmp_addr_reg[rn];
                uint64_t base, size;
                pmp_decode_napot(region, &base, &size);
                uart_puthex8(rn);
                uart_putc(' ');
                uart_puthex32(ctrl);
                uart_putc(' ');
                uart_puthex(base);
                uart_putc(' ');
                uart_puthex((~((size) - 1)));
                uart_putc('\n');
            } else if(((ctrl & PMP_A)>>PMP_A_SHIFT)!=0) {// only NAPOT region is supported
                uart_puthex8(rn);
                uart_putc(' ');
                uart_puthex32(ctrl);
                uart_puts(" not supported region\n");
            }
        }
#endif // PLF_MRT_SUPPORT
#if PLF_MRT_SUPPORT
        int rmax_mrt = scr_mrt_num_regions();
        uart_puts("MRT regions (");
        uart_putdec(rmax_mrt);
        uart_puts("):\n");
        for (unsigned long rn = 0; rn < rmax_mrt; ++rn) {
            const uint64_t ctrl = *(volatile uint64_t *)MRT_A_ENTRY_N_CTRL(rn);
            if (ctrl & MRT_ENTRY_VALID) {
                const uint64_t base =  *(volatile uint64_t *)MRT_A_ENTRY_N_ADDR(rn);
                const uint64_t mask =  *(volatile uint64_t *)MRT_A_ENTRY_N_MASK(rn);
                uart_puthex8(rn);
                uart_putc(' ');
                uart_puthex32(ctrl);
                uart_putc(' ');
                uart_puthex(base);
                uart_putc(' ');
                uart_puthex(mask);
                uart_putc('\n');
            }
        }
#endif // PLF_MRT_SUPPORT
    }
}

#if PLF_SMP_SUPPORT
#define SCBL_VER_SMP_STR " SMP"
#else
#define SCBL_VER_SMP_STR ""
#endif

const char scbl_version_str[] = "SCR loader v" FW_VER "-" FW_VER_CFG SCBL_VER_SMP_STR;
const char scbl_ver_tag_str[] = FW_VER_TAG;

extern int bsp_printf(const char *fmt, ...);
void show_header(void)
{
    uart_puts("\n");
    uart_puts(scbl_version_str);
    if (enable_dbg_out) {
        uart_puts(" (");
        uart_puts(FW_VER_TAG);
        uart_puts(")");
    }
#ifndef PLF_RUN_QUIET_MODE
    uart_puts("\n" COPYRIGHT_STR "\n");
#endif // PLF_RUN_QUIET_MODE

}

void print_op_speed(const char *prefix_str, unsigned long size, sys_tick_t timediff)
{
    unsigned time_ms = ticks2ms(timediff) + 1;
    unsigned speed = ((size * 10 / time_ms) * 1000) / 1024;

    printf("%s%u.%03u s, %u.%01u KiB/s\n",
           prefix_str,
           time_ms / 1000,
           time_ms % 1000,
           speed / 10,
           speed % 10);
}

#define LOAD_FLAG_NORMAL 0
#define LOAD_FLAG_QUIET  1

#ifdef PLF_NVRAM_KERNEL_BASE
#ifdef PLF_XQSPI_BASE
void flash_load(void)
{
    uint8_t *load_addr = (uint8_t*)PLF_MEM_BASE;
    unsigned load_size;

    printf("Copying from qspi flash 0x%08x -> %p\n",
           PLF_NVRAM_KERNEL_BASE, load_addr);

    sys_tick_t start_time = now();

    load_size = PLF_NVRAM_KERNEL_SIZE;
    xqspi_flash_fast_read(PLF_XQSPI_BASE, PLF_NVRAM_KERNEL_BASE,
        load_addr, PLF_NVRAM_KERNEL_SIZE, 1);

    print_op_speed("done, ", load_size, now() - start_time);
}

void flash_update(void)
{
    uint8_t *update_addr = (uint8_t*)PLF_MEM_BASE;
    uint32_t update_size;

    printf("This operation erase flash! Press SPACE to cancel...");
    for (unsigned i = 0; i < 9 * 10; ++i) {
        if (uart_getc_nowait() == ' ') {
            printf("\nCancelled\n");
            return;
        }
        rtc_delay_us(100000);
    }

    printf("\n\nCopying from memory to qspi flash %p -> 0x%08lx\n",
           update_addr, (unsigned long)PLF_NVRAM_KERNEL_BASE);

    sys_tick_t start_time = now();

    update_size = PLF_NVRAM_KERNEL_SIZE;

    xqspi_flash_write(PLF_XQSPI_BASE, PLF_NVRAM_KERNEL_BASE,
        update_addr, update_size, 1);

    print_op_speed("done, ", update_size, now() - start_time);
}
#define flash_update flash_update
#else // PLF_XQSPI_BASE
void flash_load(void)
{
    const unsigned COPY_BLK_SIZE = 128 * 1024;

    uint8_t *flash_addr = (uint8_t*)PLF_NVRAM_KERNEL_BASE;
    uint8_t *load_addr = (uint8_t*)PLF_MEM_BASE;
    unsigned load_size;

    printf("Copying 0x%08lx -> %p\n", (unsigned long)PLF_NVRAM_KERNEL_BASE, load_addr);

    sys_tick_t start_time = now();

    for (load_size = 0; load_size < PLF_NVRAM_KERNEL_SIZE; load_size += COPY_BLK_SIZE) {
        memcpy(load_addr, flash_addr, COPY_BLK_SIZE);
        load_addr += COPY_BLK_SIZE;
        flash_addr += COPY_BLK_SIZE;
        printf("\r%uK ", load_size / 1024);
    }

    print_op_speed("done, ", load_size, now() - start_time);
}
#endif // else PLF_XQSPI_BASE
#endif // PLF_NVRAM_KERNEL_BASE

#ifdef PLF_KERNEL_ENTRY
#define FSBL_KERNEL_ENTRY 1
#else // PLF_KERNEL_ENTRY
#if PLF_SMP_SUPPORT
#define FSBL_KERNEL_ENTRY 1
#endif // PLF_SMP_SUPPORT
#endif // PLF_KERNEL_ENTRY

#if FSBL_KERNEL_ENTRY
void kernel_start(void)
{
#ifndef PLF_UART0_SCR_RTL
    plf_con_tx_flush();
#if !PLF_TIMEOUTS_OFF
    rtc_delay_us(20000);
#endif // ! PLF_TIMEOUTS_OFF

#endif // ! PLF_UART0_SCR_RTL
    machine_init();
}
#endif // FSBL_KERNEL_ENTRY

#if defined(PLF_KERNEL_ENTRY)
#ifndef SDCARD_KERNEL_FILENAME
#define SDCARD_KERNEL_FILENAME "vmlinux.bin"
#endif // SDCARD_KERNEL_FILENAME

static void cmd_start_linux(void *arg)
{
    uart_puts("\nStarting Linux ...\n");
    kernel_start();
}
#elif PLF_SMP_SUPPORT
uintptr_t kernel_entry = 0;
static void cmd_start_smp(void *arg)
{
    kernel_entry = (uintptr_t)arg;
    uart_puts("\nStarting ...\n");
    kernel_start();
}
#endif // !PLF_MMODE_ONLY && PLF_KERNEL_ENTRY

static void do_autostart(void)
{
#ifdef PLF_NON_INTERACTIVE
    // non-interactive platforms: boot Linux immediately
#ifndef PLF_DISABLE_RTC_CHECK
    scr_rtc_check(enable_dbg_out);
#endif // !PLF_DISABLE_RTC_CHECK
    cmd_start_linux(0);
#elif defined(PLF_NO_AUTOSTART) || !(defined(PLF_AUTOSTART_NVRAM) || defined(PLF_AUTOSTART_SDCARD))
    // autostart disabled or no autostart support: continue in interactive mode
    return;
#else
    // autostart mode

    static bool autostart_enable = true;

    if (!autostart_enable)
        return;

    autostart_enable = false; // disable autorestart
    cache_flush(&autostart_enable, sizeof(autostart_enable));

    // flush uart input
    while (uart_getc_nowait() >= 0);

    if (PLF_AUTOSTART_DELAY > 0) {
        printf("Press SPACE to skip autostart... %3u", (unsigned)PLF_AUTOSTART_DELAY);
        for (unsigned i = PLF_AUTOSTART_DELAY; i > 0; --i) {
            for (unsigned j = 0; j < 100; ++j) {
                if (uart_getc_nowait() == ' ') {
                    printf("\r%40s\r", "");
                    return;
                }
                rtc_delay_us(10000);
            }
            printf("\b\b\b%3u", i - 1);
        }
    }

    printf("\r%40s\r", "");

#ifdef PLF_AUTOSTART_NVRAM
    flash_load();
    cmd_start_linux(0);
#elif defined(PLF_AUTOSTART_SDCARD)
    sd_load((void*)PLF_MEM_BASE, SDCARD_KERNEL_FILENAME, LOAD_FLAG_NORMAL);
    cmd_start_linux(0);
#else
#error "No autostart image"
#endif

#endif
}

#ifdef PLF_XMODEM_ENABLED
static void cmd_xload(void *arg)
{
    int st = xmodem_receive((uint8_t*)arg, MAX_XMODEM_RX_LEN);

    while (uart_getc_nowait() >= 0);

    if (st < 0) {
        uart_puts("\nXmodem receive error: ");
        uart_putdec(-st);
        uart_putc('\n');
    }
    else  {
        cache_flush(arg, st);

        uart_puts("\nXmodem successfully received ");
        uart_putdec(st);
        uart_puts(" bytes\n");
    }
}
#endif // PLF_XMODEM_ENABLED

#ifdef PLF_NVRAM_KERNEL_BASE
static void cmd_flash_load(void *arg)
{
    flash_load();
}

#ifdef flash_update
static void cmd_flash_update(void *arg)
{
    flash_update();
}
#endif // flash_update
#endif // PLF_NVRAM_KERNEL_BASE

static void cmd_start(void *arg)
{
    uart_puts("\n");
    plf_con_tx_flush();
    rtc_delay_us(20000);
    asm volatile ("jalr %0" :: "r"(arg));
}

static void cmd_mem_dump(void *arg)
{
    dump_mem((uintptr_t)arg, 128);
}

#if PLF_MEM_DUMP32
static void cmd_mem_dump32(void *arg)
{
    dump_mem32(((uintptr_t)arg) & ~3, 128);
}
#endif

static void cmd_mem_modify(void *arg)
{
    uart_puthex((unsigned long)arg);
    uart_puts(": ");
    unsigned long val = uart_read_hex();
    *(volatile unsigned long*)arg = val;
}

#ifdef PLF_MEM_TEST_ENABLED
static void cmd_mem_test(void *arg)
{
    sys_tick_t start_time, total_time = 0;
    uart_puts("size: ");
    unsigned long size = uart_read_hex();
    uart_puts("\n");

    // test1: direct address test
    uart_puts("Direct address test\n");

    start_time = now();

    for (volatile uintptr_t *addr = arg; addr < (uintptr_t*)(arg + size); ++addr)
        *addr = (uintptr_t)addr;

    /* for (load_size = 0; load_size < PLF_NVRAM_KERNEL_SIZE; load_size += COPY_BLK_SIZE) { */
    /*     memcpy(load_addr, flash_addr, COPY_BLK_SIZE); */
    /*     load_addr += COPY_BLK_SIZE; */
    /*     flash_addr += COPY_BLK_SIZE; */
    /*     printf("\r%uK ", load_size / 1024); */
    /* } */

    for (volatile uintptr_t *addr = arg; addr < (uintptr_t*)(arg + size); ++addr) {
        uintptr_t val = *addr;
        uintptr_t expected = (uintptr_t)addr;
        if (val != expected) {
            uart_puts("*FAIL* ");
            uart_puthex((unsigned long)addr);
            uart_puts(": read ");
            uart_puthex(val);
            uart_puts(" != expected ");
            uart_puthex(expected);
            uart_puts("\n");
            return;
        }
    }
    total_time += now() - start_time;
    print_op_speed("OK, ", size, now() - start_time);

    // test2: inverted address test
    uart_puts("Inverted address test\n");

    start_time = now();

    for (volatile uintptr_t *addr = arg; addr < (uintptr_t*)(arg + size); ++addr)
        *addr = ~(uintptr_t)addr;

    for (volatile uintptr_t *addr = arg; addr < (uintptr_t*)(arg + size); ++addr) {
        uintptr_t val = *addr;
        uintptr_t expected = ~(uintptr_t)addr;
        if (val != expected) {
            uart_puts("*FAIL* ");
            uart_puthex((unsigned long)addr);
            uart_puts(": read ");
            uart_puthex(val);
            uart_puts(" != expected ");
            uart_puthex(expected);
            uart_puts("\n");
            return;
        }
    }

    total_time += now() - start_time;
    print_op_speed("OK, ", size, now() - start_time);

    // test#: running ones

    char buf[64];
    scr_time2str(buf, sizeof(buf), total_time);
    buf[sizeof(buf) - 1] = 0;

    printf("Elapsed time: %s\n", buf);
}
#endif // PLF_MEM_TEST_ENABLED

void usage(void);

static void cmd_flip_dbg(void *arg)
{
    enable_dbg_out = !enable_dbg_out;
    if (enable_dbg_out) {
        show_header();
        hwinfo();
    } else {
        uart_putc('\n');
    }
}

#ifdef PLF_HWCNT_DUMP
/* race-free read of a 64-bit counter split across lo/hi CSRs (RV32) */
#define read_counter64(dst, lo, hi) do{			\
    uint32_t _h, _l, _h2;                               \
    do { _h = read_csr(hi); _l = read_csr(lo);          \
         _h2 = read_csr(hi); } while (_h != _h2);       \
    (dst) = ((uint64_t)_h << 32) | _l; }while(0)

static void cmd_dump_counters(void *arg)
{
    uint64_t cyc, inst, tim;

    read_counter64(cyc,  mcycle,   mcycleh);
    read_counter64(inst, minstret, minstreth);
    read_counter64(tim,  time,     timeh);

    uart_puts("HW counters:\n");
    uart_puts("  cycle:   "); uart_puthex64(cyc);  uart_putc('\n');
    uart_puts("  instret: "); uart_puthex64(inst); uart_putc('\n');
    uart_puts("  time:    "); uart_puthex64(tim);  uart_putc('\n');
}
#endif // PLF_HWCNT_DUMP

static void cmd_print_time(void *arg)
{
    char buf[64];
    sys_tick_t t = now();

    scr_time2str(buf, sizeof(buf), now());
    buf[sizeof(buf) - 1] = 0;
    printf("uptime: %lx (%s)\n", (unsigned long)t, buf);

    scr_rtc_check(enable_dbg_out);
}

static void cmd_show_commands(void *arg)
{
    usage();
}

#define SCBL_CMD_HIDDEN (1 << 31)
#define SCBL_CMD_ARG_ADDR (1 << 30)
#define SCBL_CMD_ARG_AUTOINC (1 << 29)

struct scbl_cmd {
    int key;
    const char *descr;
    void (*func)(void*);
    void *data;
};

static const struct scbl_cmd scbl_commands[] = {
#if defined(PLF_KERNEL_ENTRY)
    {'0', "start Linux", cmd_start_linux, 0},
#elif PLF_SMP_SUPPORT
    {'0' | SCBL_CMD_ARG_ADDR, "start SMP @addr", cmd_start_smp, 0},
#endif // PLF_KERNEL_ENTRY || PLF_SMP_SUPPORT
#ifdef PLF_XMODEM_ENABLED
    {'1' | SCBL_CMD_ARG_ADDR, "xload @addr", cmd_xload, 0},
#endif // PLF_XMODEM_ENABLED
#ifdef PLF_NVRAM_KERNEL_BASE
    {'v', "int flash load", cmd_flash_load, 0},
#ifdef flash_update
    {'u', "int flash update", cmd_flash_update, 0},
#endif // flash_update
#endif // PLF_NVRAM_KERNEL_BASE
    {'g' | SCBL_CMD_ARG_ADDR, "start @addr", cmd_start, 0},
    {'d' | SCBL_CMD_ARG_ADDR | SCBL_CMD_ARG_AUTOINC, "mem dump", cmd_mem_dump, (void*)128},
#if PLF_MEM_DUMP32
    {'D' | SCBL_CMD_ARG_ADDR | SCBL_CMD_ARG_AUTOINC, "mem dump (by 32-bit word)", cmd_mem_dump32, (void*)128},
#endif
    {'m' | SCBL_CMD_ARG_ADDR | SCBL_CMD_ARG_AUTOINC, "mem modify", cmd_mem_modify, (void*)(sizeof(long))},
#ifdef PLF_MEM_TEST_ENABLED
    {'t' | SCBL_CMD_ARG_ADDR, "mem test", cmd_mem_test, 0},
#endif // PLF_MEM_TEST_ENABLED
    {'T' | SCBL_CMD_HIDDEN, "current time", cmd_print_time, 0},
#ifdef PLF_HWCNT_DUMP
    {'c', "dump hw counters", cmd_dump_counters, 0},
#endif // PLF_HWCNT_DUMP
    {'!' | SCBL_CMD_HIDDEN, 0, cmd_flip_dbg, 0},
#ifdef PLF_CACHE_CFG
    {'$' | SCBL_CMD_HIDDEN, "flip L1$", cmd_flip_l1cache, 0},
#if PLF_L2CTL_BASE
    {'%' | SCBL_CMD_HIDDEN, "flip L2$", cmd_flip_l2cache, 0},
#endif // PLF_L2CTL_BASE
#endif // PLF_CACHE_CFG
    {' ' | SCBL_CMD_HIDDEN, 0, cmd_show_commands, 0},
};

void usage(void)
{
    uart_puts("\n");

    for (unsigned i = 0; i < sizeof(scbl_commands) / sizeof(*scbl_commands); ++i) {
        const struct scbl_cmd *cmd = &scbl_commands[i];
        if (((cmd->key & SCBL_CMD_HIDDEN) == 0 || enable_dbg_out) &&
            cmd->descr) {
            uart_putc(cmd->key & 0xff);
            uart_puts(": ");
            uart_puts(cmd->descr);
            uart_puts("\n");
        }
    }
}

#if PLF_SMP_SUPPORT
void init_smp(void);
#else // PLF_SMP_SUPPORT
#define init_smp() do {} while (0)
#endif // PLF_SMP_SUPPORT

#ifdef PLF_L3CTL_BASE
__attribute__((section(".data")))
 unsigned long wrong_startup_address;

 static void do_rabbit() {
	uart_puts("(\\(\\\n( -.-)\no_(\")(\")\n");
 }
#endif // PLF_L3CTL_BASE

extern int bsp_printf(const char *fmt, ...);
int main(void)
{
    scr_hwinfo_init();
    uart_init();
    scr_rtc_init();
    leds_init();

    show_header();

#ifdef PLF_XGMAC_BASE
    if(!mmio_read_allowed(PLF_XGMAC_BASE, NULL)) {
        printf("sc-bl is configured with xgmac, but xgmac is not available, panic\n");
        hart_halt();
    }
#endif // PLF_XGMAC_BASE
#ifdef PLF_XLGMAC_BASE
    if(!mmio_read_allowed(PLF_XLGMAC_BASE + 0x110, NULL)) {
        printf("sc-bl is configured with xlgmac, but xlgmac is not available, panic\n");
        hart_halt();
    }
#endif // PLF_XLGMAC_BASE
#ifndef PLF_RUN_QUIET_MODE
    hwinfo();
    ps2_init(PLF_PS2_PORT_KBD);
#endif // PLF_RUN_QUIET_MODE
    init_smp();
#ifndef PLF_RUN_QUIET_MODE
    usage();
#endif // PLF_RUN_QUIET_MODE

#ifdef PLF_L3CTL_BASE
    if(wrong_startup_address == WRONG_STARTUP_ADDRESS_POISON) {
		do_rabbit();
        uart_puts("WOW! You need to load SC-BL at 0x2000200, not 0x2000000!\n");
        uart_puts("SC-BL started to use new load address 0x2000200. "
				  "Do not worry, an older one will work fine for some time, but, please, fix your pipelines "
				  "as it will be possible.\n");
        rtc_delay_us(1000);
    }
#endif // PLF_L3CTL_BASE

    do_autostart();

    const struct scbl_cmd *prev_cmd = 0;
    void *prev_addr = 0;

    while (1) {
        uart_putc(enable_dbg_out ? '!' : ':');
        uart_putc(' ');

        int c = uart_getc();

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
