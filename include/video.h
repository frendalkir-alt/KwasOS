#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>

#define VIDEO_MEMORY 0xB8000
#define COLS 80
#define ROWS 25
#define COLOR_WHITE  0x0F
#define COLOR_GREEN  0x02
#define COLOR_YELLOW 0x0E
#define COLOR_RED    0x04
#define COLOR_CYAN   0x03
#define COLOR_GRAY   0x08

void set_cursor(int row, int col);
void gotoxy(int row, int col);
void clear_screen();
void scroll_up();
void print_char(char c, unsigned char color);
void print_string(const char *str, unsigned char color);
void delete_char();
int get_cursor_row();
int get_cursor_col();

#endif