BUILD_DIR ?= ../build

all:
	@echo "$(ANSI_GREEN)Building the initramfs tar archive$(ANSI_RESET)"
	@tar --format=ustar -cf $(BUILD_DIR)/initramfs.img -C root .