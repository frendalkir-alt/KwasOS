#include "terminal.h"
#include <string.h>
#include <stdio.h>

void terminal_init(Terminal* term) {
    memset(term->chars, ' ', TERM_ROWS * TERM_COLS);
    memset(term->fg, 7, TERM_ROWS * TERM_COLS);   // белый по умолчанию
    memset(term->bg, 0, TERM_ROWS * TERM_COLS);   // чёрный фон
    term->cursor_x = 0;
    term->cursor_y = 0;
    term->cursor_visible = 1;
}

void terminal_putchar(Terminal* term, char c) {
    if (c == '\n') {
        term->cursor_x = 0;
        term->cursor_y++;
    } else if (c == '\r') {
        term->cursor_x = 0;
    } else if (c == '\t') {
        term->cursor_x = (term->cursor_x + 8) & ~7;
    } else if (c == '\b') {
        if (term->cursor_x > 0) term->cursor_x--;
        // не стираем символ, можно добавить
    } else {
        if (term->cursor_x >= TERM_COLS) {
            term->cursor_x = 0;
            term->cursor_y++;
        }
        term->chars[term->cursor_y][term->cursor_x] = c;
        term->cursor_x++;
    }
    // Обработка переполнения строки
    if (term->cursor_x >= TERM_COLS) {
        term->cursor_x = 0;
        term->cursor_y++;
    }
    // Прокрутка
    if (term->cursor_y >= TERM_ROWS) {
        terminal_scroll(term);
        term->cursor_y = TERM_ROWS - 1;
    }
}

void terminal_scroll(Terminal* term) {
    // Сдвиг всех строк вверх на 1
    memmove(term->chars[0], term->chars[1], (TERM_ROWS - 1) * TERM_COLS);
    memmove(term->fg[0], term->fg[1], (TERM_ROWS - 1) * TERM_COLS);
    memmove(term->bg[0], term->bg[1], (TERM_ROWS - 1) * TERM_COLS);
    // Очистка последней строки
    memset(term->chars[TERM_ROWS-1], ' ', TERM_COLS);
    memset(term->fg[TERM_ROWS-1], 7, TERM_COLS);
    memset(term->bg[TERM_ROWS-1], 0, TERM_COLS);
}

void terminal_clear(Terminal* term) {
    terminal_init(term);
}

void terminal_set_cursor(Terminal* term, int x, int y) {
    if (x >= 0 && x < TERM_COLS && y >= 0 && y < TERM_ROWS) {
        term->cursor_x = x;
        term->cursor_y = y;
    }
}