CC = i686-elf-gcc
CFLAGS = -nostdlib -nostdinc -ffreestanding -std=c99 -Wall -Wextra -Iinclude
ASFLAGS = -f elf32

SRC = src/kernel.c src/video.c src/keyboard.c src/shell.c src/string.c src/reboot.c
OBJ = $(SRC:.c=.o) boot.o

all: KwasOS.bin

KwasOS.bin: $(OBJ)
	$(CC) -nostdlib -ffreestanding -T linker.ld -o $@ $^

boot.o: boot/boot.asm
	nasm $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) KwasOS.bin

run: KwasOS.bin
	qemu-system-x86_64 -kernel KwasOS.bin -m 256

.PHONY: all clean run