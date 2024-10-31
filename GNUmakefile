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

cdimage:
	@echo "Creating ISO image..."
	@mkdir -p $(BUILD_DIR)/iso_root/boot/hyper
	@cp $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/iso_root/
	@cp kernel/cfg/hyper.cfg $(BUILD_DIR)/iso_root/boot/hyper/
	@cp $(BUILD_DIR)/initramfs.img $(BUILD_DIR)/iso_root/

	@cp $(HYPER_DIR)/build/loader/hyper_iso_boot $(BUILD_DIR)/iso_root/

	@xorriso -as mkisofs \
		-o $(BUILD_DIR)/image.iso \
		-b hyper_iso_boot \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		--protective-msdos-label $(BUILD_DIR)/iso_root

	@$(HYPER_DIR)/installer/hyper_install $(BUILD_DIR)/image.iso

	@echo "ISO image created at $(BUILD_DIR)/image.iso"



hyper-bootloader:
	@rm -rf $(HYPER_DIR)/build
	@mkdir -p $(HYPER_DIR)/build
	@cd $(HYPER_DIR)/build && cmake .. -DHYPER_ARCH=i686 -DHYPER_PLATFORM=bios -DCMAKE_TOOLCHAIN_FILE=$(EXTERNAL_DIR)/toolchain_i686_elf.cmake
	@cd $(HYPER_DIR)/build && make VERBOSE=1 -j$(nproc) 

reinstall-hyper:
	@rm -rf $(HYPER_DIR)/
	@git clone https://github.com/UltraOS/Hyper.git --depth=1 --recurse-submodules $(HYPER_DIR)
	@make hyper-bootloader

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

run-iso:
	@clear	
	@qemu-system-i386 -cdrom $(BUILD_DIR)/image.iso \
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
	@rm -rf $(BUILD_DIR)/

reset:
	@make clean
	@clear
	@make

.PHONY: all kernel disk ramfs run clean reset reinstall-hyper release cdimage