#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cpu.h"
#include "memory.h"
#include "gui/window.h"
#include "devices/keyboard.h"

// Глобальный указатель на GUI для доступа из cpm
static GUI_Context* g_gui = NULL;

// Функция cpm (эмуляция CP/M) – переопределяем вывод
void cpm(i8080* const s) {
    uint16_t addr = s->d << 8 | s->e;
    if (s->c == 2) {
        // Вывод символа
        char ch = (char)s->e;
        if (g_gui) {
            gui_set_terminal_output(g_gui, ch);
        } else {
            putchar(ch);
        }
    } else if (s->c == 9) {
        // Вывод строки до '$'
        do {
            char ch = (char)readByte(s, addr++);
            if (g_gui) {
                gui_set_terminal_output(g_gui, ch);
            } else {
                putchar(ch);
            }
        } while (readByte(s, addr) != '$');
    }
}

// Остальные функции из оригинального main.c (загрузка файла и т.д.)
static inline int load_file(i8080* const s, const char *file_to_load) {
    FILE* f = fopen(file_to_load, "rb");
    if (f == NULL) {
        perror(file_to_load);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    rewind(f);
    size_t file_read = fread(&s->memory[0x100], sizeof(uint8_t), 0x10000 - 0x100, f);
    if (file_read != file_size) {
        fprintf(stderr, "CAN'T READ: \"%s\" INTO MEMORY!\n", file_to_load);
        fclose(f);
        return 1;
    }
    fclose(f);
    return 0;
}

static inline void execute_file(i8080* const s, const char* file_to_load) {
    init_state(s);
    memset(s->memory, 0, 0x10000);
    if (load_file(s, file_to_load) != 0) {
        fprintf(stderr, "Failed to load file, exiting.\n");
        return;
    }
    s->memory[0x0005] = 0xC9;   // RET для эмуляции вызовов CP/M
    s->pc = 0x0100;
    printf("\n*** FILE LOADED: %s\n", file_to_load);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program.com>\n", argv[0]);
        return 1;
    }

    // Инициализация GUI
    GUI_Context gui;
    if (gui_init(&gui, "8080 Virtual Machine", 1024, 768) != 0) {
        fprintf(stderr, "GUI initialization failed\n");
        return 1;
    }
    g_gui = &gui;

    // Инициализация CPU
    i8080 cpu;
    execute_file(&cpu, argv[1]);

    // Главный цикл
    const int instructions_per_frame = 5000;
    int step_mode = 0;  // 0 - непрерывно, 1 - пошагово

    while (gui.running) {
        gui_handle_events(&gui, &cpu);

        // Выполняем инструкции
        if (!step_mode) {
            for (int i = 0; i < instructions_per_frame; i++) {
                if (cpu.pc == 0x0000) {
                    // Программа завершилась
                    gui.running = false;
                    break;
                }
                cpu_step(&cpu);
            }
        } else {
            // В пошаговом режиме инструкции выполняются по F10
            // (уже обработано в gui_handle_events)
        }

        // Рендеринг
        gui_render(&gui, &cpu);

        // Задержка для контроля FPS
        SDL_Delay(10);
    }

    gui_close(&gui);
    return 0;
}
