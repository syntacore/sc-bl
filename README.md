Syntacore RISC-V first stage bootloader
========================================

About
--------------

This package contains the Syntacore first stage bootloader (SCBL) for SCR SDK platforms.

Prerequisites
--------------

RISC-V GCC toolchain is required to compile the software. You can use pre-built binaries or build the toolchain from scratch.

#### Using pre-built binary tools

Pre-built RISC-V GCC toolchain and OpenOCD binaries are available to download from http://syntacore.com/page/products/sw-tools. Download the archive (*.tar.gz* for Linux, *.zip* for Windows) for your platform, extract the archive to your preferred directory and update the PATH environment variable as described in **Set environment variables** section.

#### Building tools from source

You can build the RISC-V toolchain from sources.

Build procedure is verified at the Ubuntu 14.04 LTS and Ubuntu 16.04 LTS distributions.

    sudo apt-get install autoconf automake libmpc-dev libmpfr-dev libgmp-dev gawk bison flex texinfo libtool make g++ pkg-config libexpat1-dev zlib1g-dev
    git clone https://github.com/riscv/riscv-gnu-toolchain.git
    cd riscv-gnu-toolchain
    git checkout a71fc539850f8dacf232fc580743b946c376014b
    git submodule update --init --recursive
    ./configure --prefix=<YOUR_INSTALL_PATH> --enable-multilib
    make

More detailed instructions on how to prepare and build the toolchain can be found in https://github.com/riscv/riscv-tools/blob/master/README.md.

#### Set environment variables

Add the <YOUR_INSTALL_PATH>/bin folder to the PATH environment variable:

    export PATH=$PATH:<YOUR_INSTALL_PATH>/bin

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

After the build process complets the target SCBL's files are created in the top level source directory, named *build.<target_platform>*.
