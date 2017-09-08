Syntacore RISC-V first stage bootloader
========================================

About
--------------

This package contains the Syntacore first stage bootloader (SCBL) for SCR SDK platforms.

Prerequisites
--------------

    riscv64 multilib toolchain
or

    riscv32 toolchain

Supported target platforms
--------------

    arty_scr1
    de10lite_scr1

Build SCBL
--------------

We assume that the RISC-V environment variable is set to the RISC-V tools install path, and that the riscv-gnu-toolchain package is installed.
To build SCBL execute:

    $ make CROSS_COMPILE=riscv64-unknown-elf- PLATFORM=<target_platform>
or

    $ make CROSS_COMPILE=riscv32-unknown-elf- PLATFORM=<target_platform>


After the build process complets the target SCBL's files are created in the top level source directory, named *build.<target_platform>*.
