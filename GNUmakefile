override MAKEFLAGS += -rRs
CORES := $(shell nproc)
BUILD_DIR := $(abspath ./build)
EXTERNAL_DIR := $(abspath ./external)
HYPER_DIR := $(abspath $(EXTERNAL_DIR)/hyper)

include config.mk

all: kernel ramfs disk run

kernel:
	@make -C kernel/ BUILD_DIR=$(BUILD_DIR)

ramfs:
	@make -C ramfs/ BUILD_DIR=$(BUILD_DIR)

disk:
	@dd if=/dev/zero bs=1M count=0 seek=64 of=$(BUILD_DIR)/image.hdd
	@parted $(BUILD_DIR)/image.hdd mklabel msdos
	@parted $(BUILD_DIR)/image.hdd mkpart primary fat32 1MiB 100%
	@mformat -i $(BUILD_DIR)/image.hdd@@1M

	@$(HYPER_DIR)/installer/hyper_install $(BUILD_DIR)/image.hdd

	@mcopy -i $(BUILD_DIR)/image.hdd@@1M $(BUILD_DIR)/kernel.bin kernel/cfg/hyper.cfg $(BUILD_DIR)/initramfs.img ::/

hyper-bootloader:
	@rm -rf $(HYPER_DIR)/build
	@mkdir -p $(HYPER_DIR)/build
	@cd $(HYPER_DIR)/build && cmake .. -DHYPER_ARCH=i686 -DHYPER_PLATFORM=bios -DCMAKE_TOOLCHAIN_FILE=$(EXTERNAL_DIR)/toolchain_i686_elf.cmake
	@cd $(HYPER_DIR)/build && make VERBOSE=1 -j$(nproc) 

reinstall-hyper:
	@rm -rf $(HYPER_DIR)/
	@git clone https://github.com/UltraOS/Hyper.git --depth=1 --recurse-submodules $(HYPER_DIR)
	@make hyper-bootloader

installer: hyper-bootloader release
	@echo "Creating installer floppy image..."
	@rm -f $(BUILD_DIR)/floppy.img
	@dd if=/dev/zero of=$(BUILD_DIR)/floppy.img bs=512 count=2880
	@mformat -i $(BUILD_DIR)/floppy.img -f 1440 ::
	@mcopy -i $(BUILD_DIR)/floppy.img $(BUILD_DIR)/kernel.bin kernel/cfg/hyper.cfg $(HYPER_DIR)/installer/hyper_install.exe ::/
	@echo "Installer floppy image created at $(BUILD_DIR)/floppy.img"

installer-run: installer
	@rm -f $(BUILD_DIR)/installer_test.img
	@qemu-img create -f raw $(BUILD_DIR)/installer_test.img 1G
	@parted $(BUILD_DIR)/installer_test.img mklabel msdos
	@parted $(BUILD_DIR)/installer_test.img mkpart primary fat32 1MiB 100%
	@mformat -i $(BUILD_DIR)/installer_test.img@@1M

	qemu-system-i386 -fda Dos6.22.img -drive file=$(BUILD_DIR)/floppy.img,format=raw,if=floppy,index=1 \
	                 -hda $(BUILD_DIR)/installer_test.img -boot b -m 32

run:
	@clear	
	@qemu-system-i386 -drive format=raw,file=$(BUILD_DIR)/image.hdd,if=ide,index=0 \
		-m 64M -cpu pentium \
		-machine pc-i440fx-2.9,acpi=off \
		-device cirrus-vga \
		-debugcon stdio \
		--no-reboot --no-shutdown \
		-serial file:$(BUILD_DIR)/serial_output.txt \
		-d int \
		-D $(BUILD_DIR)/qemu_interrupt.log

debug:
	@clear
	@qemu-system-i386 -drive format=raw,file=$(BUILD_DIR)/image.hdd,if=ide,index=0 \
		-m 64M -cpu pentium \
		-machine pc-i440fx-2.9,acpi=off \
		-device cirrus-vga \
		-debugcon stdio \
		--no-reboot --no-shutdown \
		-serial file:$(BUILD_DIR)/serial_output.txt \
		-d int \
		-D $(BUILD_DIR)/qemu_interrupt.log \
		-s -S & \
		sleep 2 && \
		gdb -ex "file $(BUILD_DIR)/kernel.bin" -ex "target remote localhost:1234"

release: kernel
	@$(STRIP) $(BUILD_DIR)/kernel.bin

clean:
	@clear
	@make -C kernel/ clean
	@rm -rf $(BUILD_DIR)/image.hdd $(BUILD_DIR)/image.iso iso_root/ $(BUILD_DIR)/initramfs.img
	@rm -rf $(BUILD_DIR)/serial_output.txt $(BUILD_DIR)/qemu_log.txt

reset:
	@make clean
	@clear
	@make

.PHONY: all kernel disk ramfs run clean reset reinstall-hyper release installer installer-run