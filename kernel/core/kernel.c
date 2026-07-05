// kernel/core/kernel.c

#include "video.h"
#include "shell.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include <timer.h>
#include <fat32.h>
#include <ata.h>

void kmain(void) {
    clear_screen();
    print_string("Initializing interrupts...\n", COLOR_CYAN);

    idt_init();
    pic_remap();
    init_keyboard();

    print_string("Initializing timer...", COLOR_CYAN);

    timer_init();

    pic_unmask(1);
    pic_unmask(0);

    ata_init();
    fat32_init();

    __asm__ volatile ("sti");

    clear_screen();
    print_string("KwasOS 0.1.7-beta\n", COLOR_GREEN);
    print_string("(c) kirka ALL RIGHTS RESERVED\n", COLOR_YELLOW);
    print_string("Type 'help' for commands.\n", COLOR_CYAN);

    char buffer[256];
    while (1) {
        print_string("# ", COLOR_YELLOW);
        readline(buffer, sizeof(buffer));
        int was_clear = process_command(buffer);
        if (!was_clear && get_cursor_col() != 0) {
            print_char('\n', COLOR_WHITE);
        }
    }
}