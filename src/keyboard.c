// src/keyboard.c
#include "keyboard.h"
#include "video.h"

static int shift_pressed = 0;

static const char scancode_to_ascii[] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0,   0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,   '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' '
};

static const char scancode_to_shift[] = {
    0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,   0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' '
};

int get_key() {
    unsigned char scancode;
    while (1) {
        unsigned char status;
        __asm__ volatile ("inb $0x64, %0" : "=a"(status));
        if (!(status & 0x01)) continue;
        __asm__ volatile ("inb $0x60, %0" : "=a"(scancode));

        // Обработка Shift
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 1;
            continue;
        }
        if (scancode == (0x2A | 0x80) || scancode == (0x36 | 0x80)) {
            shift_pressed = 0;
            continue;
        }

        // Обработка расширенных клавиш (0xE0)
        if (scancode == 0xE0) {
            unsigned char scancode2;
            do {
                __asm__ volatile ("inb $0x64, %0" : "=a"(status));
            } while (!(status & 0x01));
            __asm__ volatile ("inb $0x60, %0" : "=a"(scancode2));
            if (scancode2 & 0x80) continue;  // отпускание
            // Возвращаем специальные коды
            if (scancode2 == 0x48) return KEY_UP;
            if (scancode2 == 0x50) return KEY_DOWN;
            if (scancode2 == 0x4B) return KEY_LEFT;
            if (scancode2 == 0x4D) return KEY_RIGHT;
            continue;
        }

        // Отпускания обычных клавиш пропускаем
        if (scancode & 0x80) continue;

        // Enter и Backspace
        if (scancode == 0x1C) return '\n';
        if (scancode == 0x0E) return 0x08;

        // Печатные символы
        if (scancode < sizeof(scancode_to_ascii)) {
            char ascii = shift_pressed ? scancode_to_shift[scancode] : scancode_to_ascii[scancode];
            if (ascii != 0) return (int)(unsigned char)ascii;
        }
    }
}