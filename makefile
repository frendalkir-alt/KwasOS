# makefile
# Используем i686-elf-gcc для ядра, но загрузчик собирается отдельно
CC = i686-elf-gcc
CFLAGS = -nostdlib -nostdinc -ffreestanding -std=c99 -Wall -Wextra -Iinclude
ASFLAGS = -f elf32

SRC = src/kernel.c src/video.c src/keyboard.c src/shell.c src/string.c src/reboot.c
OBJ = $(SRC:.c=.o)

# Ядро как плоский бинарник (без ELF)
KERNEL_ELF = kernel.elf
KERNEL_BIN = kernel.bin

all: floppy.img

# Компиляция ядра в ELF (для удобства отладки) и затем извлечение .text в raw binary
$(KERNEL_ELF): $(OBJ)
	$(CC) -nostdlib -ffreestanding -T linker.ld -o $@ $^

$(KERNEL_BIN): $(KERNEL_ELF)
	i686-elf-objcopy -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Загрузчик (бинарник 512 байт)
KwasBOOT.bin: KwasBOOT/boot.asm
	nasm -f bin $< -o $@

# Образ дискеты 1.44 МБ с загрузчиком и ядром
floppy.img: KwasBOOT.bin $(KERNEL_BIN)
	dd if=/dev/zero of=floppy.img bs=1024 count=1440
	dd if=KwasBOOT.bin of=floppy.img bs=512 count=1 conv=notrunc
	dd if=$(KERNEL_BIN) of=floppy.img bs=512 seek=1 conv=notrunc

clean:
	rm -f $(OBJ) $(KERNEL_ELF) $(KERNEL_BIN) KwasBOOT.bin floppy.img

run: floppy.img
	qemu-system-x86_64 -fda floppy.img -m 256

.PHONY: all clean run