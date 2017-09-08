/// Syntacore SCR* framework
///
/// @copyright (C) Syntacore 2015-2017. All rights reserved.
/// @author mn-sc
///
/// @brief platform specific configurations

#ifndef PLATFORM_DE10LITE_SCR1_CONFIG_H
#define PLATFORM_DE10LITE_SCR1_CONFIG_H

#define PLF_CPU_NAME "SCR1"
#define PLF_IMPL_STR "Syntacore FPGA"

// RTC timebase: 1 MHZ, internal
#define PLF_RTC_TIMEBASE 1000000
// sys clk freq, MHz
#define PLF_SYS_FREQ     20000000
// cpu clk freq
#define PLF_CPU_FREQ     PLF_SYS_FREQ

//----------------------
// memory configuration
//----------------------
#define PLF_MEM_BASE     (0)
#define PLF_MEM_SIZE     (4*1024*1024)
#define PLF_MEM_ATTR     0
#define PLF_MEM_NAME     "SDRAM"

#define PLF_TCM_BASE     (0xf0000000)
#define PLF_TCM_SIZE     (64*1024)
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
#define PLF_BLD_ID_ADDR  (PLF_MMIO_BASE + 0)

// LEDs
#define PLF_PINLED_ADDR  (PLF_MMIO_BASE + 0x21000)
#define PLF_PINLED_NUM   10
#define PLF_PINLED_INV   0
#define PLF_PINLED_NAME "LED"

#define PLF_HEXLED_ADDR (PLF_MMIO_BASE + 0x20000)
#define PLF_HEXLED_ADDR_MAP                     \
    {(PLF_HEXLED_ADDR + 0x20), 8},              \
    {(PLF_HEXLED_ADDR + 0x20), 0},              \
    {(PLF_HEXLED_ADDR + 0x10), 8},              \
    {(PLF_HEXLED_ADDR + 0x10), 0},              \
    {(PLF_HEXLED_ADDR + 0x0), 8},               \
    {(PLF_HEXLED_ADDR + 0x0), 0}

#define PLF_HEXLED_PORT_WIDTH 2
#define PLF_HEXLED_NUM 6
#define PLF_HEXLED_INV 0xff
#define PLF_HEXLED_NAME "Hex LED"

#define HEXLED_SEG_A 0x01
#define HEXLED_SEG_B 0x02
#define HEXLED_SEG_C 0x04
#define HEXLED_SEG_D 0x08
#define HEXLED_SEG_E 0x10
#define HEXLED_SEG_F 0x20
#define HEXLED_SEG_G 0x40
#define HEXLED_SEG_P 0x80

// DIP switches
#define PLF_DIP_ADDR (PLF_MMIO_BASE + 0x22000)
#define PLF_DIP_NUM  10
#define PLF_DIP_NAME "DIP sw"

// external interrupt lines

#define PLF_IRQ_MAP                      \
        [0 ... 31] = ~0,                 \
        [12] = PLF_UART0_IRQ

#endif // PLATFORM_DE10LITE_SCR1_CONFIG_H
