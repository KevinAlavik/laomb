# config.mk

export ANSI_GREEN := \033[0;32m
export ANSI_RESET := \033[0m

export DEFAULT_CC := $(HOME)/toolchain-i586/i586-elf/bin/i586-elf-gcc
export DEFAULT_LD := $(HOME)/toolchain-i586/i586-elf/bin/i586-elf-ld
export DEFAULT_AS := $(HOME)/toolchain-i586/i586-elf/bin/i586-elf-as
export DEFAULT_AR := $(HOME)/toolchain-i586/i586-elf/bin/i586-elf-ar
export DEFAULT_CXX := $(HOME)/toolchain-i586/i586-elf/bin/i586-elf-g++
export OBJDUMP := $(HOME)/toolchain-i586/i586-elf/bin/i586-elf-objdump
export OBJCOPY := $(HOME)/toolchain-i586/i586-elf/bin/i586-elf-objcopy
export NM := $(HOME)/toolchain-i586/i586-elf/bin/i586-elf-nm

export DEFAULT_CFLAGS := -std=c23
export DEFAULT_CXXFLAGS := -std=c++23

export DEFAULT_CCFLAGS := -g -O2 -pipe -Wall -Wextra -Werror -fno-stack-protector -ffreestanding -march=pentium -mtune=pentium

export DEFAULT_NASMFLAGS := -Wall -g
export DEFAULT_LDFLAGS := -g -O2

export KERNEL_CFLAGS := $(DEFAULT_CFLAGS) $(DEFAULT_CCFLAGS) -fPIE -m32 -march=i586 -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -ffreestanding \
	-fno-omit-frame-pointer -fno-lto -nostdlib -nostartfiles \
	-I . -I ./inc -I ./cfg

export KERNEL_CXXFLAGS := $(DEFAULT_CXXFLAGS) $(DEFAULT_CCFLAGS) -fPIE -m32 -march=i586 -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -ffreestanding \
	-Wno-delete-non-virtual-dtor -nostdinc++ -fno-omit-frame-pointer -fno-use-cxa-atexit -fno-exceptions -fno-rtti \
	-I . -I ./inc -I ./cfg

export KERNEL_LDFLAGS := $(DEFAULT_LDFLAGS) -ffreestanding -nostdlib -nostartfiles -T cfg/linker.ld -static -lgcc
export KERNEL_NASMFLAGS := $(DEFAULT_NASMFLAGS) -f elf32