#include "video.h"

static int cursor_row = 0;
static int cursor_col = 0;

void set_cursor(int row, int col) {
    unsigned short pos = row * COLS + col;
    __asm__ volatile ("outb %0, %1" : : "a"((unsigned char)0x0F), "d"((unsigned short)0x3D4));
    __asm__ volatile ("outb %0, %1" : : "a"((unsigned char)(pos & 0xFF)), "d"((unsigned short)0x3D5));
    __asm__ volatile ("outb %0, %1" : : "a"((unsigned char)0x0E), "d"((unsigned short)0x3D4));
    __asm__ volatile ("outb %0, %1" : : "a"((unsigned char)((pos >> 8) & 0xFF)), "d"((unsigned short)0x3D5));
}

void clear_screen() {
    char *video = (char*) VIDEO_MEMORY;
    for (int i = 0; i < COLS * ROWS * 2; i += 2) {
        video[i] = ' ';
        video[i + 1] = COLOR_WHITE;
    }
    cursor_row = 0;
    cursor_col = 0;
    set_cursor(cursor_row, cursor_col);
}

void scroll_up() {
    char *video = (char*) VIDEO_MEMORY;
    for (int i = 0; i < (ROWS-1) * COLS * 2; i++) {
        video[i] = video[i + COLS * 2];
    }
    for (int i = (ROWS-1) * COLS * 2; i < ROWS * COLS * 2; i += 2) {
        video[i] = ' ';
        video[i+1] = COLOR_WHITE;
    }
    if (cursor_row > 0) cursor_row--;
}

void print_char(char c, unsigned char color) {
    if (c == '\n') {
        cursor_row++;
        cursor_col = 0;
    } else {
        char *video = (char*) VIDEO_MEMORY;
        int index = (cursor_row * COLS + cursor_col) * 2;
        video[index] = c;
        video[index + 1] = color;
        cursor_col++;
        if (cursor_col >= COLS) {
            cursor_col = 0;
            cursor_row++;
        }
    }
    if (cursor_row >= ROWS) {
        scroll_up();
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
        char *video = (char*) VIDEO_MEMORY;
        int index = (cursor_row * COLS + cursor_col) * 2;
        video[index] = ' ';
        video[index + 1] = COLOR_WHITE;
        set_cursor(cursor_row, cursor_col);
    } else if (cursor_row > 0) {
        cursor_row--;
        cursor_col = COLS - 1;
        char *video = (char*) VIDEO_MEMORY;
        int index = (cursor_row * COLS + cursor_col) * 2;
        video[index] = ' ';
        video[index + 1] = COLOR_WHITE;
        set_cursor(cursor_row, cursor_col);
    }
}

void gotoxy(int row, int col) {
    cursor_row = row;
    cursor_col = col;
    set_cursor(row, col);
}

int get_cursor_row() { return cursor_row; }
int get_cursor_col() { return cursor_col; }