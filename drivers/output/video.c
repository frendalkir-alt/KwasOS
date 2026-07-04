// drivers/output/video.c

#include "video.h"
#include "io.h"

static int cursor_row = 0;
static int cursor_col = 0;
static volatile uint16_t *video = (volatile uint16_t*) VIDEO_MEMORY;

void set_cursor(int row, int col) {
    cursor_row = row;
    cursor_col = col;
    uint16_t pos = row * COLS + col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void gotoxy(int row, int col) {
    if (row >= 0 && row < ROWS && col >= 0 && col < COLS)
        set_cursor(row, col);
}

void clear_screen() {
    for (int i = 0; i < ROWS * COLS; i++) {
        video[i] = ' ' | (COLOR_WHITE << 8);
    }
    cursor_row = 0;
    cursor_col = 0;
    set_cursor(0, 0);
}

void scroll_up() {
    for (int row = 1; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            video[(row - 1) * COLS + col] = video[row * COLS + col];
        }
    }
    int last_row = ROWS - 1;
    for (int col = 0; col < COLS; col++) {
        video[last_row * COLS + col] = ' ' | (COLOR_WHITE << 8);
    }
}

void print_char(char c, unsigned char color) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        if (cursor_row >= ROWS) {
            scroll_up();
            cursor_row = ROWS - 1;
        }
        set_cursor(cursor_row, cursor_col);
        return;
    }
    if (c == '\r')
        return;

    int index = cursor_row * COLS + cursor_col;
    video[index] = (uint16_t)((uint16_t)c | ((uint16_t)color << 8));
    cursor_col++;
    if (cursor_col >= COLS) {
        cursor_col = 0;
        cursor_row++;
        if (cursor_row >= ROWS) {
            scroll_up();
            cursor_row = ROWS - 1;
        }
    }
    set_cursor(cursor_row, cursor_col);
}

void print_string(const char *str, unsigned char color) {
    while (*str) {
        print_char(*str++, color);
    }
}

void delete_char() {
    if (cursor_col > 0) {
        cursor_col--;
        int index = cursor_row * COLS + cursor_col;
        video[index] = ' ' | (COLOR_WHITE << 8);
        set_cursor(cursor_row, cursor_col);
    } else if (cursor_row > 0) {
        cursor_row--;
        cursor_col = COLS - 1;
        int index = cursor_row * COLS + cursor_col;
        video[index] = ' ' | (COLOR_WHITE << 8);
        set_cursor(cursor_row, cursor_col);
    }
}

void print_int(int num, unsigned char color) {
    char buf[32];
    int i = 0;
    int is_negative = 0;

    if (num == 0) {
        print_char('0', color);
        return;
    }
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    if (is_negative) {
        buf[i++] = '-';
    }
    while (i > 0) {
        print_char(buf[--i], color);
    }
}

int get_cursor_row() { return cursor_row; }
int get_cursor_col() { return cursor_col; }