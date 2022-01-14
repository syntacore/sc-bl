/// Syntacore SCR* framework
///
/// @copyright (C) Syntacore 2015-2021. All rights reserved.
/// @author mn-sc
///
/// @brief platform specific configurations (Arria V GX FPGA Starter Kit)

#ifndef PLATFORM_A5_SCR1_CONFIG_H
#define PLATFORM_A5_SCR1_CONFIG_H

#define PLF_CPU_NAME "SCR1"
#define PLF_IMPL_STR "Syntacore FPGA"

// RTC timebase: 1 MHZ
#define PLF_RTC_TIMEBASE 1000000
// sys clk freq, MHz
#define PLF_SYS_FREQ     30000000
// cpu clk freq
#define PLF_CPU_FREQ_ADDR   (0xff002000)
#define PLF_CPU_FREQ_BASE    ((volatile uint32_t*)(PLF_CPU_FREQ_ADDR))
#define PLF_CPU_FREQ        ((uint32_t)(*PLF_CPU_FREQ_BASE))

//----------------------
// memory configuration
//----------------------
#define PLF_MEM_BASE     (0)
#define PLF_MEM_SIZE     (256*1024*1024)
#define PLF_MEM_ATTR     0
#define PLF_MEM_NAME     "DDR"

#define PLF_TCM_BASE     (0xf0000000)
#define PLF_TCM_SIZE     (128*1024)
#define PLF_TCM_ATTR     0
#define PLF_TCM_NAME     "TCM"

#define PLF_MTIMER_BASE  (0xf0040000)
#define PLF_MTIMER_SIZE  (0x1000)
#define PLF_MTIMER_ATTR  0
#define PLF_MTIMER_NAME  "MTimer"

#define PLF_MMIO_BASE    (0xff000000)
#define PLF_MMIO_SIZE    (0x100000)
#define PLF_MMIO_ATTR    0
#define PLF_MMIO_NAME    "MMIO"

#define PLF_OCRAM_BASE   (0xffff0000)
#define PLF_OCRAM_SIZE   (64*1024)
#define PLF_OCRAM_ATTR   0
#define PLF_OCRAM_NAME   "On-Chip RAM"

#define PLF_MEM_MAP                                                     \
    {PLF_MEM_BASE, PLF_MEM_SIZE, PLF_MEM_ATTR, PLF_MEM_NAME},           \
    {PLF_TCM_BASE, PLF_TCM_SIZE, PLF_TCM_ATTR, PLF_TCM_NAME},           \
    {PLF_MTIMER_BASE, PLF_MTIMER_SIZE, PLF_MTIMER_ATTR, PLF_MTIMER_NAME}, \
    {PLF_MMIO_BASE, PLF_MMIO_SIZE, PLF_MMIO_ATTR, PLF_MMIO_NAME},       \
    {PLF_OCRAM_BASE, PLF_OCRAM_SIZE, PLF_OCRAM_ATTR, PLF_OCRAM_NAME}

// FPGA UART port
#define PLF_UART0_BASE   (PLF_MMIO_BASE + 0x10000)
#define PLF_UART0_16550
#define PLF_UART0_IRQ 0

// FPGA build ID
#define PLF_BLD_ID_ADDR     (PLF_MMIO_BASE + 0x1000)
// FPGA system ID - SOC_ID
#define PLF_SOC_ID_ADDR     (PLF_MMIO_BASE + 0)
#define PLF_SYS_ID_ADDR     PLF_SOC_ID_ADDR

// external interrupt lines

#define PLF_IRQ_MAP                      \
        [0 ... 31] = ~0,                 \
        [0] = PLF_UART0_IRQ

#endif // PLATFORM_A5_SCR1_CONFIG_H
