// src/kernel.c
#include "video.h"
#include "shell.h"

/**
 * Точка входа в ядро.
 * Вызывается из загрузчика после перехода в защищённый режим.
 * Не принимает аргументов (в отличие от версии с Multiboot).
 */
void kmain(void) {
    // Очищаем экран и выводим приветствие
    clear_screen();
    print_string("KwasOS 0.1.6\n", COLOR_GREEN);
    print_string("Type 'help' for commands.\n", COLOR_CYAN);

    // Основной цикл командной оболочки
    char buffer[256];
    while (1) {
        print_string("$ ", COLOR_YELLOW);
        readline(buffer, sizeof(buffer));
        int was_clear = process_command(buffer);
        if (!was_clear) {
            if (get_cursor_col() != 0) {
                print_char('\n', COLOR_WHITE);
            }
        }
    }
}