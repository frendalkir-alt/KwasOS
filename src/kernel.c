// src/kernel.c
#include "video.h"
#include "shell.h"

void kmain(unsigned int magic, unsigned int addr) {
    (void)magic; (void)addr;

    clear_screen();
    print_string("KwasOS 0.1\n", COLOR_GREEN);
    print_string("Type 'help' for commands.\n", COLOR_CYAN);

    char buffer[256];

    while (1) {
        print_string("$ ", COLOR_YELLOW);
        readline(buffer, sizeof(buffer));
        int was_clear = process_command(buffer);
        if (!was_clear) {
            // Если курсор не в начале строки, добавим перевод
            if (get_cursor_col() != 0) {
                print_char('\n', COLOR_WHITE);
            }
        }
    }
}