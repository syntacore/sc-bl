/// Syntacore SCR* framework
///
/// @copyright (C) Syntacore 2015-2017. All rights reserved.
/// @author mn-sc
///
/// @brief platform specific configurations
#ifndef PLATFORM_TANG_PRIMER_20_K_SCR1_CONFIG_H
#define PLATFORM_TANG_PRIMER_20_K_SCR1_CONFIG_H
#define PLF_CPU_NAME "SCR1"
#define PLF_IMPL_STR "Syntacore FPGA"
// RTC timebase: 1 MHZ, internal
#define PLF_RTC_TIMEBASE 1000000
// sys clk freq, MHz
#define PLF_SYS_FREQ     27000000
// cpu clk freq
#define PLF_CPU_FREQ     PLF_SYS_FREQ
//----------------------
// memory configuration
//----------------------
#define PLF_TCM_BASE     (0xF0000000)
#define PLF_TCM_SIZE     (4*1024)
#define PLF_TCM_ATTR     0
#define PLF_TCM_NAME     "TCM"
#define PLF_MTIMER_BASE  (0xF0040000)
#define PLF_MTIMER_SIZE  (0x1000)
#define PLF_MTIMER_ATTR  0
#define PLF_MTIMER_NAME  "MTimer"
#define PLF_MMIO_BASE    (0xFF000000)
#define PLF_MMIO_SIZE    (0x100000)
#define PLF_MMIO_ATTR    0
#define PLF_MMIO_NAME    "MMIO"
#define PLF_OCROM_BASE   (0xFFFF0000)
#define PLF_OCROM_SIZE   (32*1024)
#define PLF_OCROM_ATTR   0
#define PLF_OCROM_NAME   "On-Chip ROM"
#define PLF_MEM_MAP                                                     \
    {PLF_TCM_BASE, PLF_TCM_SIZE, PLF_TCM_ATTR, PLF_TCM_NAME},           \
    {PLF_MTIMER_BASE, PLF_MTIMER_SIZE, PLF_MTIMER_ATTR, PLF_MTIMER_NAME}, \
    {PLF_MMIO_BASE, PLF_MMIO_SIZE, PLF_MMIO_ATTR, PLF_MMIO_NAME},       \
    {PLF_OCROM_BASE, PLF_OCROM_SIZE, PLF_OCROM_ATTR, PLF_OCROM_NAME}
// FPGA UART port
#define PLF_UART0_BASE   (PLF_MMIO_BASE + 0x10000)
#define PLF_UART0_16550
#define PLF_UART0_IRQ 0

#define PLF_IRQ_MAP                      \
        [0 ... 31] = ~0,                 \
        [12] = PLF_UART0_IRQ
#endif // PLATFORM_DE10LITE_SCR1_CONFIG_H