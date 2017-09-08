/// Syntacore SCR* infra
///
/// @copyright (C) Syntacore 2015-2017. All rights reserved.
/// @author mn-sc
///
/// @brief SCR* infra startup code

#include "platform_config.h"

#define INT_SM_SOFTWARE     1
#define INT_MM_SOFTWARE     3
#define INT_SM_TIMER        5
#define INT_MM_TIMER        7
#define INT_SM_EXTERNAL     9
#define INT_MM_EXTERNAL     11
#define INT_MM_MEMORY_FAULT 15

#define EXC_UM_ECALL        8
#define EXC_SM_ECALL        9
#define EXC_MM_ECALL        11
#define EXC_PF_FETCH        12
#define EXC_PF_LOAD         13
#define EXC_PF_STORE        15

    .altmacro
    .macro memcpy src_beg, src_end, dst, tmp_reg
    LOCAL memcpy_1, memcpy_2
    j    memcpy_2
memcpy_1:
    lw   \tmp_reg, (\src_beg)
    sw   \tmp_reg, (\dst)
    add  \src_beg, \src_beg, 4
    add  \dst, \dst, 4
memcpy_2:
    bltu \src_beg, \src_end, memcpy_1
    .endm

    .macro memset dst_beg, dst_end, val_reg
    LOCAL memset_1, memset_2
    j    memset_2
memset_1:
    sw   \val_reg, (\dst_beg)
    add  \dst_beg, \dst_beg, 4
memset_2:
    bltu \dst_beg, \dst_end, memset_1
    .endm

    .option norvc

    .global main
    .global mcall_trap
    .global bad_trap, bad_trap_handler
    .global _hart_halt

    .section ".startup","ax",@progbits

_crt_start:
    ## reset mstatus: MPP=3, MPIE=1, MIE=0
    li   t0, 0x1880
    csrw mstatus, t0
    ## setup MTVEC
    la   t0, __trap_hander_1v10
    csrw 0x305, t0

    ## setup MIE, MIP
    csrw mip, zero
    li t0, (1 << INT_MM_TIMER)
    csrw mie, t0

    ## setup gp
    la gp, __global_pointer$

    csrr a0, mhartid
    bnez a0, _hart_halt

    # init .bss
    la    t0, __BSS_START__
    la    t1, __BSS_END__
    memset t0, t1, zero

    ## init HART0 stack, sp
    la    sp, __TLS0_BASE__
    mv    tp, sp

    ## init HART0 TLS
    la    t0, _tdata_begin
    la    t1, _tdata_end
    mv    t2, tp
    memcpy t0, t1, t2, a0
    la    t0, _tbss_end
    sub   t0, t0, t1
    add   t0, t0, t2
    memset t2, t0, zero

    # boot hart 0
    csrw mscratch, sp

    mv  a0, zero
    mv  a1, zero
    jal main

# halt hart
_hart_halt:
    ## disable timer interrupts
    li t0, (1 << INT_MM_TIMER)
    csrc mie, t0
1:
    wfi
    j 1b

    .balign 0x20

interrupt_handler:

unexpected_int_handler:
    j bad_trap

    .balign 0x20

handle_trap_in_machine_mode:
    ## save regs
    sw    a2,12*4(sp)
    sw    a3,13*4(sp)
    csrr  a2, cycle
    csrr  a3, instret

    sw    ra, 1*4(sp)
    csrrw a1, mscratch, sp # a1 <- user sp
    sw    a1, 2*4(sp)      # sp
    sw    gp, 3*4(sp)
    sw    tp, 4*4(sp)
    sw    t0, 5*4(sp)
    sw    t1, 6*4(sp)
    sw    t2, 7*4(sp)
    sw    s0, 8*4(sp)
    sw    s1, 9*4(sp)
    sw    a4,14*4(sp)
    sw    a5,15*4(sp)
    sw    a6,16*4(sp)
    sw    a7,17*4(sp)
    sw    s2,18*4(sp)
    sw    s3,19*4(sp)
    sw    s4,20*4(sp)
    sw    s5,21*4(sp)
    sw    s6,22*4(sp)
    sw    s7,23*4(sp)
    sw    s8,24*4(sp)
    sw    s9,25*4(sp)
    sw    s10,26*4(sp)
    sw    s11,27*4(sp)
    sw    t3,28*4(sp)
    sw    t4,29*4(sp)
    sw    t5,30*4(sp)
    sw    t6,31*4(sp)

    la    gp, __global_pointer$
    mv    a1, sp    # a1 <- regs

    li    t0, EXC_UM_ECALL
    beq   t0, a0, 1f
    li    t0, EXC_MM_ECALL
    bne   t0, a0, .Lbad_trap
1:

    ## invoke trap handler
    mv    tp, sp
    jal   mcall_trap

    ##  restore regs
    lw    ra, 1*4(sp)
    lw    gp, 3*4(sp)
    lw    tp, 4*4(sp)
    lw    t0, 5*4(sp)
    lw    t1, 6*4(sp)
    lw    t2, 7*4(sp)
    lw    s0, 8*4(sp)
    lw    s1, 9*4(sp)
    lw    a1,11*4(sp)
    lw    a2,12*4(sp)
    lw    a3,13*4(sp)
    lw    a4,14*4(sp)
    lw    a5,15*4(sp)
    lw    a6,16*4(sp)
    lw    a7,17*4(sp)
    lw    s2,18*4(sp)
    lw    s3,19*4(sp)
    lw    s4,20*4(sp)
    lw    s5,21*4(sp)
    lw    s6,22*4(sp)
    lw    s7,23*4(sp)
    lw    s8,24*4(sp)
    lw    s9,25*4(sp)
    lw    s10,26*4(sp)
    lw    s11,27*4(sp)
    lw    t3,28*4(sp)
    lw    t4,29*4(sp)
    lw    t5,30*4(sp)
    lw    t6,31*4(sp)

    ## return
    lw   a0, 10*4(sp)
    lw   sp, 2*4(sp)
    mret

.Lbad_trap:
    j bad_trap_handler

    .align 4
__trap_hander_1v10:
    csrrw sp, mscratch, sp
    sw    a0, 10*4(sp)
    sw    a1, 11*4(sp)

    csrr  a0, mcause
    bltz  a0, interrupt_handler
    j     handle_trap_in_machine_mode

    .section ".vectors","ax",@progbits
    .global _start
_start:
    j     _crt_start
  nop
  .align 6
    ## entry point for NMIs
__nmi_trap_entry:
    j     bad_trap
    .align 7
__trap_entry:
    j     __trap_hander_1v10
