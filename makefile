# Определение ОС
UNAME_S := $(shell uname -s)

# Настройка инструментов в зависимости от ОС
ifeq ($(UNAME_S),Linux)
    CC = gcc
    OBJCOPY = objcopy
    QEMU = qemu-system-x86_64
    XORRISO = xorriso
    NASM = nasm
endif

ifeq ($(UNAME_S),Darwin)
    # macOS
    X8664_ELF_GCC := $(shell command -v x86_64-elf-gcc 2>/dev/null)
    ifeq ($(strip $(X8664_ELF_GCC)),)
        $(error "x86_64-elf-gcc not found. Please install it: brew install x86_64-elf-gcc")
    endif
    CC = x86_64-elf-gcc
    OBJCOPY = x86_64-elf-objcopy
    QEMU = qemu-system-x86_64
    XORRISO = xorriso
    NASM = nasm
endif

# Windows (MSYS, MINGW, Cygwin)
ifneq (,$(findstring MINGW,$(UNAME_S)))
    CC = x86_64-elf-gcc
    OBJCOPY = x86_64-elf-objcopy
    QEMU = qemu-system-x86_64
    XORRISO = xorriso
    NASM = nasm
endif
ifneq (,$(findstring MSYS,$(UNAME_S)))
    CC = x86_64-elf-gcc
    OBJCOPY = x86_64-elf-objcopy
    QEMU = qemu-system-x86_64
    XORRISO = xorriso
    NASM = nasm
endif
ifneq (,$(findstring CYGWIN,$(UNAME_S)))
    CC = x86_64-elf-gcc
    OBJCOPY = x86_64-elf-objcopy
    QEMU = qemu-system-x86_64
    XORRISO = xorriso
    NASM = nasm
endif

# Если переменные не определены, выдать ошибку
ifeq ($(CC),)
    $(error "Unsupported OS: $(UNAME_S). Please install required tools manually.")
endif

# Флаги компиляции
CFLAGS = -nostdlib -nostdinc -ffreestanding -std=c99 -Wall -Wextra -Iinclude -m64

# Папки и цели
BUILD_DIR = build
ISO_ROOT = $(BUILD_DIR)/iso_root

SRC = src/kernel.c src/video.c src/keyboard.c src/shell.c src/string.c src/reboot.c
OBJ = $(addprefix $(BUILD_DIR)/, $(notdir $(SRC:.c=.o)))

KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
BOOT_BIN = $(BUILD_DIR)/KwasBOOT.bin
FLOPPY_IMG = $(BUILD_DIR)/floppy.img
ISO_IMG = KwasOS.iso

all: $(ISO_IMG)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(OBJ)
	$(CC) -nostdlib -ffreestanding -T linker.ld -o $@ $^

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

$(BOOT_BIN): KwasBOOT/boot.asm | $(BUILD_DIR)
	$(NASM) -f bin $< -o $@

$(FLOPPY_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	dd if=/dev/zero of=$@ bs=1024 count=1440 2>/dev/null
	dd if=$(BOOT_BIN) of=$@ bs=512 count=1 conv=notrunc 2>/dev/null
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null

$(ISO_IMG): $(FLOPPY_IMG)
	mkdir -p $(ISO_ROOT)
	cp $(FLOPPY_IMG) $(ISO_ROOT)/floppy.img
	$(XORRISO) -as mkisofs -b floppy.img -c boot.catalog -o $@ $(ISO_ROOT)
	rm -rf $(ISO_ROOT)

run: $(ISO_IMG)
	$(QEMU) -cdrom $(ISO_IMG) -m 256 -cpu qemu64

floppy: $(FLOPPY_IMG)
run-floppy: $(FLOPPY_IMG)
	$(QEMU) -fda $(FLOPPY_IMG) -m 256 -cpu qemu64

check-deps:
	@echo "Checking dependencies..."
	@command -v $(CC) >/dev/null 2>&1 || { echo "Error: $(CC) not found"; exit 1; }
	@command -v $(OBJCOPY) >/dev/null 2>&1 || { echo "Error: $(OBJCOPY) not found"; exit 1; }
	@command -v $(NASM) >/dev/null 2>&1 || { echo "Error: $(NASM) not found"; exit 1; }
	@command -v $(XORRISO) >/dev/null 2>&1 || { echo "Error: $(XORRISO) not found"; exit 1; }
	@command -v $(QEMU) >/dev/null 2>&1 || { echo "Error: $(QEMU) not found"; exit 1; }
	@echo "All dependencies satisfied."

clean:
	rm -rf $(BUILD_DIR) $(ISO_IMG)

.PHONY: all clean run run-floppy floppy check-deps