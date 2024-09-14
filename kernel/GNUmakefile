BUILD_DIR ?= ../build

override KERNEL := kernel.bin

override CFILES := $(shell find -L * -type f -name '*.c')
override ASFILES := $(shell find -L * -type f -name '*.S')
override NASMFILES := $(shell find -L * -type f -name '*.asm')
override ASFILES := $(shell find -L * -type f -name '*.S')
override HEADER_DEPS := $(addprefix $(BUILD_DIR)/kernel/obj/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))
override CXXFILES := $(shell find -L * -type f -name '*.cpp')
override OBJ := $(addprefix $(BUILD_DIR)/kernel/obj/,$(CFILES:.c=.c.o) $(CXXFILES:.cpp=.cpp.o) $(ASFILES:.S=.S.o) $(NASMFILES:.asm=.asm.o))

override CC := $(DEFAULT_CC)
override CXX := $(DEFAULT_CXX)
override LD := $(DEFAULT_LD)

.PHONY: all
all: $(BUILD_DIR)/$(KERNEL)

cfg/ultra_protocol.h:
	@curl -Lo $@ https://raw.githubusercontent.com/UltraOS/UltraProtocol/69235d363c077a090a9b7a313ed8c6f73e4a4807/ultra_protocol.h cfg/ultra_protocol.h

$(BUILD_DIR)/$(KERNEL): GNUmakefile cfg/linker.ld $(OBJ) cfg/ultra_protocol.h
	@mkdir -p "$$(dirname $@)"
	@echo -e "$(ANSI_GREEN)LINKING$(ANSI_RESET) $@"
	@$(CC) $(OBJ) $(KERNEL_LDFLAGS) -o $@

-include $(HEADER_DEPS)

$(BUILD_DIR)/kernel/obj/%.c.o: %.c GNUmakefile
	@mkdir -p "$$(dirname $@)"
	@echo -e "$(ANSI_GREEN)COMPILING$(ANSI_RESET) $<"
	@$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel/obj/%.cpp.o: %.cpp GNUmakefile
	@mkdir -p "$$(dirname $@)"
	@echo -e "$(ANSI_GREEN)COMPILING$(ANSI_RESET) $<"
	@$(CXX) $(KERNEL_CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel/obj/%.S.o: %.S GNUmakefile
	@mkdir -p "$$(dirname $@)"
	@echo -e "$(ANSI_GREEN)ASSEMBLING$(ANSI_RESET) $<"
	@$(CC) $(KERNEL_CFLAGS) $(KERNEL_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel/obj/%.asm.o: %.asm GNUmakefile
	@mkdir -p "$$(dirname $@)"
	@echo -e "$(ANSI_GREEN)ASSEMBLING$(ANSI_RESET) $<"
	@nasm $(KERNEL_NASMFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/kernel $(BUILD_DIR)/$(KERNEL)

.PHONY: distclean
distclean: clean
	rm -f limine.h
