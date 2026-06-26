# Компилятор для x86-64
CC = x86_64-elf-gcc
CFLAGS = -nostdlib -nostdinc -ffreestanding -std=c99 -Wall -Wextra -Iinclude -m64

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
	x86_64-elf-objcopy -O binary $< $@

$(BOOT_BIN): KwasBOOT/boot.asm | $(BUILD_DIR)
	nasm -f bin $< -o $@

$(FLOPPY_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	dd if=/dev/zero of=$@ bs=1024 count=1440 2>/dev/null
	dd if=$(BOOT_BIN) of=$@ bs=512 count=1 conv=notrunc 2>/dev/null
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null

$(ISO_IMG): $(FLOPPY_IMG)
	mkdir -p $(ISO_ROOT)
	cp $(FLOPPY_IMG) $(ISO_ROOT)/floppy.img
	xorriso -as mkisofs -b floppy.img -c boot.catalog -o $@ $(ISO_ROOT)
	rm -rf $(ISO_ROOT)

run: $(ISO_IMG)
	qemu-system-x86_64 -cdrom $(ISO_IMG) -m 256 -cpu qemu64

floppy: $(FLOPPY_IMG)
run-floppy: $(FLOPPY_IMG)
	qemu-system-x86_64 -fda $(FLOPPY_IMG) -m 256 -cpu qemu64

clean:
	rm -rf $(BUILD_DIR) $(ISO_IMG)

.PHONY: all clean run run-floppy floppy