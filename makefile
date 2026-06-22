# Makefile
CFLAGS = -m32 -nostdlib -nostdinc -ffreestanding -std=c99 -Wall -Wextra -Iinclude
LDFLAGS = -m elf_i386 -T linker.ld
ASFLAGS = -f elf32

# Исходные файлы
SRC = src/kernel.c src/video.c src/keyboard.c src/shell.c src/string.c src/reboot.c
OBJ = $(SRC:.c=.o) boot.o

all: KwasOS.iso

KwasOS.bin: $(OBJ)
	ld $(LDFLAGS) -o $@ $^

boot.o: boot/boot.asm
	nasm $(ASFLAGS) $< -o $@

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

KwasOS.iso: KwasOS.bin grub.cfg
	mkdir -p iso/boot/grub
	cp KwasOS.bin iso/boot/
	cp grub.cfg iso/boot/grub/
	grub-mkrescue -o KwasOS.iso iso/

clean:
	rm -f $(OBJ) KwasOS.bin KwasOS.iso
	rm -rf iso/

run: KwasOS.iso
	qemu-system-x86_64 -cdrom KwasOS.iso -m 256

.PHONY: all clean run