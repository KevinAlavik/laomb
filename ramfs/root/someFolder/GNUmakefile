CORES := $(shell nproc)
# CORES := 2

all: kernel ramfs disk run

kernel:
	@make -C kernel/

ramfs:
	@mkdir -p build
	@tar --format=ustar -cf build/initramfs.img -C ramfs .

disk:
	@dd if=/dev/zero bs=1M count=0 seek=64 of=build/image.hdd
	@sgdisk build/image.hdd -n 1:2048 -t 1:ef00

	@./limine/limine bios-install build/image.hdd
	@mformat -i build/image.hdd@@1M
	@mmd -i build/image.hdd@@1M ::/EFI ::/EFI/BOOT

	@mcopy -i build/image.hdd@@1M build/kernel.bin kernel/cfg/limine.cfg limine/limine-bios.sys ::/
	@mcopy -i build/image.hdd@@1M limine/BOOTX64.EFI ::/EFI/BOOT
	@mcopy -i build/image.hdd@@1M limine/BOOTIA32.EFI ::/EFI/BOOT

	@mcopy -i build/image.hdd@@1M build/initramfs.img ::/

	@echo "Example File Content" > build/example.txt
	@mcopy -i build/image.hdd@@1M build/example.txt ::/

limine:
	@git clone https://github.com/limine-bootloader/limine.git --branch=v7.x-binary --depth=1
	@make -C limine

reinstall-limine:
	@rm -rf limine/
	@make limine

run:
	@clear
	@qemu-system-x86_64 -drive format=raw,file=build/image.hdd \
			-m 4G -enable-kvm -cpu host -smp $(CORES) -M q35 \
			-debugcon stdio \
			--no-reboot \
			-serial file:build/serial_output.txt \
			-monitor file:build/monitor_output.txt \
			-d int -M smm=off \
			-device pci-bridge,chassis_nr=3,id=b2 \
			-D build/qemu_log.txt -d guest_errors,cpu_reset
 #--no-shutdown
clean:
	@clear
	@make -C kernel/ clean
	@rm -rf build/image.hdd build/image.iso iso_root/ build/initramfs.img
	@rm -rf build/serial_output.txt

reset:
	@make clean
	@clear
	@make

.PHONY: all kernel disk ramfs run clean reset reinstall-limine
