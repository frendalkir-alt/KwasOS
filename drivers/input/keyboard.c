#include "keyboard.h"
#include "video.h"
#include "io.h"

#define KEYBOARD_DATA_PORT  0x60
#define KEYBOARD_STATUS_PORT 0x64
#define BUFFER_SIZE 128

static volatile unsigned char buffer[BUFFER_SIZE];
static volatile int head = 0;
static volatile int tail = 0;
static int shift_pressed = 0;
static int extended = 0;   // флаг для расширенных скан-кодов (0xE0)

static const char ascii_table[] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0,   0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,   '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' '
};

static const char shift_table[] = {
    0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,   0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' '
};

void init_keyboard(void) {
    head = tail = 0;
    shift_pressed = 0;
    extended = 0;
}

void keyboard_handler(void) {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    if (!(status & 0x01)) return;
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    // Обработка расширенного префикса 0xE0
    if (scancode == 0xE0) {
        extended = 1;
        return;
    }

    // Если это код отпускания (бит 7 = 1), игнорируем, но сбрасываем extended
    if (scancode & 0x80) {
        // Для Shift отпускание обрабатываем отдельно
        if (scancode == (0x2A | 0x80) || scancode == (0x36 | 0x80)) {
            shift_pressed = 0;
        }
        extended = 0;
        return;
    }

    // Обработка Shift (нажатие)
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        extended = 0;
        return;
    }

    // Обработка расширенных клавиш (стрелки, и т.д.)
    if (extended) {
        extended = 0;
        int keycode = 0;
        switch (scancode) {
            case 0x48: keycode = KEY_UP; break;
            case 0x50: keycode = KEY_DOWN; break;
            case 0x4B: keycode = KEY_LEFT; break;
            case 0x4D: keycode = KEY_RIGHT; break;
            default: return; // не обрабатываем другие расширенные
        }
        // Кладём в буфер специальный код
        int next = (head + 1) % BUFFER_SIZE;
        if (next != tail) {
            buffer[head] = (unsigned char)keycode; // но keycode > 255, не влезет в char
            // Лучше хранить в отдельном массиве int, но для простоты пока игнорируем
            // или используем два байта? Проще оставить обработку в get_key через ожидание, но это ломает прерывания.
            // Пока просто не поддерживаем стрелки, чтобы не усложнять.
        }
        return;
    }

    // Обычные клавиши
    char ascii = shift_pressed ? shift_table[scancode] : ascii_table[scancode];
    if (ascii) {
        int next = (head + 1) % BUFFER_SIZE;
        if (next != tail) {
            buffer[head] = ascii;
            head = next;
        }
        return;
    }

    // Специальные клавиши (Enter, Backspace)
    if (scancode == 0x1C) { // Enter
        int next = (head + 1) % BUFFER_SIZE;
        if (next != tail) {
            buffer[head] = '\n';
            head = next;
        }
        return;
    }
    if (scancode == 0x0E) { // Backspace
        int next = (head + 1) % BUFFER_SIZE;
        if (next != tail) {
            buffer[head] = '\b'; // или 0x08
            head = next;
        }
        return;
    }
}

int get_key(void) {
    while (1) {
        if (head != tail) {
            int ch = buffer[tail];
            tail = (tail + 1) % BUFFER_SIZE;
            return ch;
        }
        __asm__ volatile ("hlt");
    }
}