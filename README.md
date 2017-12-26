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

Target name | Description | Clock, MHz | TCM | RAM
------ | ----------- | --------- | ----- | -----
arty_scr1     | Digilent Arty A7  | 25 | 64K | -
de10lite_scr1 | Terasic DE10-lite | 20 | 64K | 64M
a5_scr1       | Intel Arria V GX FPGA Starter Kit | 30 | 128K | 256M

Build SCBL
--------------

We assume that the directory *bin*, where the riscv-gnu-toolchain compiler binary has been installed, is a part of the PATH environment variable.

To build SCBL execute:

    $ make CROSS_COMPILE=riscv64-unknown-elf- PLATFORM=<target_platform>
or

    $ make CROSS_COMPILE=riscv32-unknown-elf- PLATFORM=<target_platform>


After the build process complets the target SCBL's files are created in the top level source directory, named *build.<target_platform>*.
