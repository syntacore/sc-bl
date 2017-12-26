### Syntacore SCR* infra
###
### @copyright (C) Syntacore 2015-2016. All rights reserved.
### @author mn-sc
###
### @brief SC boot loader Makefile

.PHONY: all riscv clean hex

all: riscv hex

PLATFORM ?= arty_scr1

apps = scbl

ld-script?=scbl.ld

FLAGS_MARCH ?= rv32im
FLAGS_MABI ?= ilp32

PLATFORM_HDR=plf_$(PLATFORM).h

CROSS_PATH ?=/opt/riscv/bin
CROSS_COMPILE ?= $(CROSS_PATH)$(if $(CROSS_PATH),/)riscv64-unknown-elf-

CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wa,-march=$(FLAGS_MARCH) -march=$(FLAGS_MARCH) -mabi=${FLAGS_MABI} -mstrict-align -std=gnu99 $(INCLUDE_DIRS) $(C_OPT_FLAGS) -Wall -Werror

LIBS = -lgcc -lc
LD = $(CC)
LDFLAGS = -march=$(FLAGS_MARCH) -mabi=${FLAGS_MABI} -static -T common/$(ld-script) -Xlinker -nostdlib -nostartfiles -ffast-math -Wl,--gc-sections -Wl,-Map=$(@:.elf=.map)
OBJDUMP = $(CROSS_COMPILE)objdump -w -x -s -S
OBJCOPY = $(CROSS_COMPILE)objcopy

ifdef PLATFORM
CFLAGS += -DPLATFORM=$(PLATFORM) -DPLATFORM_HDR=\"$(PLATFORM_HDR)\"
plf_build_siffix = .$(PLATFORM)
endif

build_dir ?= build$(plf_build_siffix)

C_OPT_FLAGS += -O2 -ffast-math -fomit-frame-pointer -fno-exceptions \
               -fno-asynchronous-unwind-tables -fno-unwind-tables \
               -fdata-sections -ffunction-sections -fno-common -fno-builtin-printf

ASM_DEFINES += -D__ASSEMBLY__=1
INCLUDE_DIRS  += -Icommon -I. -Isrc

c_src = uart.c xmodem.c init.c trap.c leds.c printf.c
asm_src = startup.S

c_objs = $(patsubst %.c, $(build_dir)/%.o, $(c_src))
asm_objs = $(patsubst %.S, $(build_dir)/%.o, $(asm_src))
app_objs = $(c_objs) $(asm_objs)

test_objs = $(patsubst %, $(build_dir)/%.o, $(basename $(apps)))

apps_elf = $(patsubst %, $(build_dir)/%.elf, $(basename $(apps)))
apps_hex = $(patsubst %, $(build_dir)/%.hex, $(basename $(apps)))

$(apps_elf): $(build_dir)/%.elf: $(build_dir)/%.o $(app_objs)
	@echo "LD\t$@"
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(OBJDUMP) $@ > $(@:.elf=.dump)
	$(OBJCOPY) -Obinary -S $@ $(@:.elf=.bin)

$(build_dir)/%.o: src/%.c | $(build_dir)
	@echo "CC\t$<"
	@$(CC) $(CFLAGS) $(C_DEFINES) -c $< -o $@

$(build_dir)/%.o: src/%.S | $(build_dir)
	@echo "AS\t$<"
	@$(CC) $(CFLAGS) $(ASM_DEFINES) -c $< -o $@

$(build_dir)/%.o: common/%.c | $(build_dir)
	@echo "CC\t$<"
	@$(CC) $(CFLAGS) $(C_DEFINES) -c $< -o $@

$(build_dir)/%.o: common/%.S | $(build_dir)
	@echo "AS\t$<"
	@$(CC) $(CFLAGS) $(ASM_DEFINES) -c $< -o $@

$(build_dir)/%.o: %.c | $(build_dir)
	@echo "CC\t$<"
	@$(CC) $(CFLAGS) $(C_DEFINES) -c $< -o $@

$(build_dir)/%.o: %.S | $(build_dir)
	@echo "AS\t$<"
	@$(CC) $(CFLAGS) $(ASM_DEFINES) -c $< -o $@

# make Xilinx *.mem and Altera *.hex files
$(apps_hex): $(build_dir)/%.hex: $(build_dir)/%.elf
	echo "@00000000" > $(@:.hex=.mem) && hexdump -v -e '4/1 "%02x" "\n"' $(@:.hex=.bin) >> $(@:.hex=.mem)
	./mk_altera_hex.sh $(@:.hex=.bin) $@

riscv: $(apps_elf)

hex: $(apps_hex)

$(build_dir):
	mkdir -p $(build_dir)

clean:
	rm -rf $(build_dir)

deep_clean:
	rm -rf build*
