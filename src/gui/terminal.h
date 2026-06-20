#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>
#include <stdint.h>

#define TERM_ROWS 25
#define TERM_COLS 80

typedef struct {
    uint8_t chars[TERM_ROWS][TERM_COLS];
    uint8_t fg[TERM_ROWS][TERM_COLS];
    uint8_t bg[TERM_ROWS][TERM_COLS];
    int cursor_x, cursor_y;
    bool cursor_visible;
} Terminal;

void terminal_init(Terminal* term);
void terminal_putchar(Terminal* term, char c);
void terminal_clear(Terminal* term);
void terminal_set_cursor(Terminal* term, int x, int y);
void terminal_scroll(Terminal* term);

#endif
