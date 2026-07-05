#include <panic.h>
#include <video.h>
#include <stdint.h>

static void print_hex(uint32_t num, unsigned char color) {
    char hex[] = "0123456789ABCDEF";
    print_char('0', color);
    print_char('x', color);
    // Выводим 8 цифр (для 32-бит)
    for (int i = 28; i >= 0; i -= 4) {
        print_char(hex[(num >> i) & 0xF], color);
    }
}

void kwas_panic(enum panic_code code, const char* message) {
    volatile uint16_t* vga = (volatile uint16_t*)VIDEO_MEMORY;
    for (int i = 0; i < ROWS * COLS; i++) {
        vga[i] = ' ' | (COLOR_WHITE_ON_RED << 8);
    }

    const char* title = "!!! KWASS SPILLED !!!";
    int title_len = 23;
    int title_col = (COLS - title_len) / 2;
    int row = 3;
    int pos = row * COLS + title_col;
    for (int i = 0; i < title_len; i++) {
        vga[pos + i] = title[i] | (COLOR_WHITE_ON_RED << 8);
    }

    row += 2;
    const char* code_label = " Error code: ";
    int code_label_len = 12;
    int col = (COLS - (code_label_len + 10)) / 2; // 10 символов для 0xXXXXXXXX
    pos = row * COLS + col;
    for (int i = 0; i < code_label_len; i++) {
        vga[pos + i] = code_label[i] | (COLOR_WHITE_ON_RED << 8);
    }
    // Выводим код в hex
    char hex_buf[11]; // "0xXXXXXXXX\0"
    hex_buf[0] = '0';
    hex_buf[1] = 'x';
    for (int i = 0; i < 8; i++) {
        int digit = (code >> (28 - i * 4)) & 0xF;
        hex_buf[2 + i] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
    }
    hex_buf[10] = '\0';
    for (int i = 0; i < 10; i++) {
        vga[pos + code_label_len + i] = hex_buf[i] | (COLOR_WHITE_ON_RED << 8);
    }

    row += 2;
    const char* prefix = "Message: ";
    int prefix_len = 11;
    int msg_len = 0;
    while (message[msg_len]) msg_len++;
    int total_len = prefix_len + msg_len;
    col = (COLS - total_len) / 2;
    pos = row * COLS + col;
    for (int i = 0; i < prefix_len; i++) {
        vga[pos + i] = prefix[i] | (COLOR_WHITE_ON_RED << 8);
    }
    for (int i = 0; i < msg_len; i++) {
        vga[pos + prefix_len + i] = message[i] | (COLOR_WHITE_ON_RED << 8);
    }

    row += 3;
    const char* halt_msg = "System halted. Reboot computer.";
    int halt_len = 0;
    while (halt_msg[halt_len]) halt_len++;
    col = (COLS - halt_len) / 2;
    pos = row * COLS + col;
    for (int i = 0; i < halt_len; i++) {
        vga[pos + i] = halt_msg[i] | (COLOR_WHITE_ON_RED << 8);
    }

    while (1) {
        __asm__ volatile ("hlt");
    }
}