// kernel/core/shell.c

#include "shell.h"
#include "video.h"
#include "keyboard.h"
#include "string.h"
#include "reboot.h"
#include <timer.h>
#include <fat32.h>
#include <panic.h>

#define HISTORY_SIZE 16
#define MAX_CMD_LEN 256

static char history[HISTORY_SIZE][MAX_CMD_LEN];
static int history_count = 0;
static int history_pos = 0;

#define INPUT_START_COL 2

void readline(char *buffer, int max_len) {
    int pos = 0;
    buffer[0] = '\0';
    history_pos = history_count;
    
    int start_row = get_cursor_row();
    
    while (1) {
        int key = get_key();
        
        if (key == '\n' || key == 0x0D) {
            buffer[pos] = '\0';
            print_char('\n', COLOR_WHITE);
            
            if (pos > 0) {
                if (history_count == 0 || strcmp(buffer, history[history_count - 1]) != 0) {
                    if (history_count < HISTORY_SIZE) {
                        int i;
                        for (i = 0; i <= pos && i < MAX_CMD_LEN - 1; i++)
                            history[history_count][i] = buffer[i];
                        history[history_count][pos] = '\0';
                        history_count++;
                    } else {
                        for (int i = 0; i < HISTORY_SIZE - 1; i++) {
                            for (int j = 0; j < MAX_CMD_LEN; j++)
                                history[i][j] = history[i + 1][j];
                        }
                        for (int i = 0; i <= pos && i < MAX_CMD_LEN - 1; i++)
                            history[HISTORY_SIZE - 1][i] = buffer[i];
                        history[HISTORY_SIZE - 1][pos] = '\0';
                    }
                }
            }
            
            history_pos = history_count;
            return;
        }
        else if (key == 0x08 || key == '\b') {
            if (pos > 0) {
                pos--;
                delete_char();
                buffer[pos] = '\0';
            }
        }
        else if (key == KEY_UP) {
            if (history_pos > 0) {
                history_pos--;
                gotoxy(start_row, INPUT_START_COL);
                for (int i = 0; i < COLS - INPUT_START_COL; i++) {
                    print_char(' ', COLOR_WHITE);
                }
                gotoxy(start_row, INPUT_START_COL);
                
                const char *hist_str = history[history_pos];
                int i = 0;
                while (hist_str[i] && i < max_len - 1) {
                    buffer[i] = hist_str[i];
                    print_char(hist_str[i], COLOR_WHITE);
                    i++;
                }
                pos = i;
                buffer[pos] = '\0';
            }
        }
        else if (key == KEY_DOWN) {
            if (history_pos < history_count) {
                history_pos++;
                gotoxy(start_row, INPUT_START_COL);
                for (int i = 0; i < COLS - INPUT_START_COL; i++) {
                    print_char(' ', COLOR_WHITE);
                }
                gotoxy(start_row, INPUT_START_COL);
                
                if (history_pos == history_count) {
                    pos = 0;
                    buffer[0] = '\0';
                } else {
                    const char *hist_str = history[history_pos];
                    int i = 0;
                    while (hist_str[i] && i < max_len - 1) {
                        buffer[i] = hist_str[i];
                        print_char(hist_str[i], COLOR_WHITE);
                        i++;
                    }
                    pos = i;
                    buffer[pos] = '\0';
                }
            }
        }
        else if (key >= 0x20 && key <= 0x7E && pos < max_len - 1) {
            buffer[pos++] = (char)key;
            print_char((char)key, COLOR_WHITE);
        }
    }
}

int process_command(char *cmd) {
    while (*cmd == ' ') cmd++;

    if (strcmp(cmd, "help") == 0) {
        print_string("System commands:\n", COLOR_YELLOW);
        print_string("  help     - Show this help\n", COLOR_WHITE);
        print_string("  echo <text> ( > <file>)     - Print text or print it to file\n", COLOR_WHITE);
        print_string("  ver     - Show OS version\n", COLOR_WHITE);
        print_string("  ticks     - Show system uptime in ticks (100 Hz)\n", COLOR_WHITE);
        print_string("  uptime     - Show how many seconds have passed since the OS was started\n", COLOR_WHITE);
        print_string("  cls     - Clear the screen\n", COLOR_WHITE);
        print_string("  reboot      - Reboot the system\n", COLOR_WHITE);
        print_string("  shutdown     - Turn off the system\n", COLOR_WHITE);
        print_string("  panic     - cause system panic - CAUTION\n", COLOR_WHITE);
        print_string("File system commands:\n", COLOR_YELLOW);
        print_string("  ls    - Show all directories on / of disk\n", COLOR_WHITE);
        print_string("  cat     - Show the file contents\n", COLOR_WHITE);
        print_string("  rm <file>     - delete a file\n", COLOR_WHITE);
        print_string("  format     - format the disk (FAT32) - CAUTION\n", COLOR_WHITE);
        return 0;
    }
    else if (strcmp(cmd, "cls") == 0) {
        clear_screen();
        print_string("KwasOS 0.1.7-beta\n", COLOR_GREEN);
        print_string("(c) kirka ALL RIGHTS RESERVED\n", COLOR_YELLOW);
        print_string("Type 'help' for commands.\n", COLOR_CYAN);
        return 1;
    }
    else if (strcmp(cmd, "reboot") == 0) {
        print_string("Rebooting...\n", COLOR_RED);
        reboot();
        return 0;
    }
    else if (strcmp(cmd, "ver") == 0) {
        print_string("KwasOS v0.1.7-beta\n", COLOR_GREEN);
        print_string("(c) kirka ALL RIGHTS RESERVED\n", COLOR_YELLOW);
        return 0;
    }
    else if (strncmp(cmd, "echo ", 5) == 0) {
        const char* rest = cmd + 5;
        // Ищем '>'
        char* redirect_pos = NULL;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == '>') {
                redirect_pos = (char*)rest + i;
                break;
            }
        }
        if (redirect_pos) {
            // Разделяем строку: содержимое до '>' и имя файла после
            // Пропускаем пробелы перед '>'
            char* content_end = redirect_pos;
            while (content_end > rest && *(content_end-1) == ' ') content_end--;
            // Копируем содержимое
            char content[256];
            int len = content_end - rest;
            if (len > 255) len = 255;
            for (int i = 0; i < len; i++) content[i] = rest[i];
            content[len] = '\0';

            // Ищем имя файла после '>'
            char* filename_start = redirect_pos + 1;
            while (*filename_start == ' ') filename_start++;
            // Убираем пробелы в конце имени
            char* filename_end = filename_start;
            while (*filename_end && *filename_end != ' ') filename_end++;
            char filename[256];
            len = filename_end - filename_start;
            if (len > 255) len = 255;
            for (int i = 0; i < len; i++) filename[i] = filename_start[i];
            filename[len] = '\0';

            // Вызываем запись
            fat32_write_file(filename, content);
        } else {
            // Обычный echo без перенаправления – выводим текст
            print_string(rest, COLOR_WHITE);
            print_char('\n', COLOR_WHITE);
        }
        return 0;
    }
    else if (*cmd == '\0') {
        return 0;
    }
    else if (strcmp(cmd, "ticks") == 0) {
        print_string("System ticks: ", COLOR_WHITE);
        print_int(get_ticks(), COLOR_WHITE);
        print_char('\n', COLOR_WHITE);
        return 0;
    }
    else if (strcmp(cmd, "uptime") == 0) {
        uint64_t sec = system_ticks / 100;
        print_string("Uptime: ", COLOR_WHITE);
        print_int(sec, COLOR_WHITE);
        print_string(" seconds\n", COLOR_WHITE);
        return 0;
    }
    else if (strcmp(cmd, "shutdown") == 0) {
        print_string("Shutting down...\n", COLOR_WHITE);
        shutdown();
        return 1;
    }
    else if (strcmp(cmd, "ls") == 0) {
        fat32_ls();
        return 0;
    }
    else if (strncmp(cmd, "cat ", 4) == 0) {
        const char* filename = cmd + 4;
        while (*filename == ' ') filename++;
        fat32_cat(filename);
        return 0;
    }
    else if (strcmp(cmd, "panic") == 0) {
        kwas_panic(PANIC_USER, "The user caused a panic");
        return 0;
    }
    else if (strncmp(cmd, "rm ", 3) == 0) {
        const char* filename = cmd + 3;
        while (*filename == ' ') filename++;
        fat32_delete_file(filename);
        return 0;
    }
    else if (strcmp(cmd, "format") == 0) {
        print_string("WARNING: ", COLOR_RED);
        print_string("This will erase all data on the disk!\n", COLOR_YELLOW);
        print_string("Are you sure? (y/N): ", COLOR_WHITE);
        int key = get_key();
        print_char(key, COLOR_WHITE);
        print_char('\n', COLOR_WHITE);
        if (key == 'y' || key == 'Y') {
            fat32_format();
        } else {
            print_string("Format cancelled.\n", COLOR_WHITE);
        }
        return 0;
    }
    else if (strcmp(cmd, "disk_info") == 0) {
        uint64_t size = fat32_get_disk_size();
        if (size) {
            print_string("Disk size: ", COLOR_WHITE);
            print_int(size, COLOR_WHITE);
            print_string(" sectors (", COLOR_WHITE);
            print_int(size / 2 / 1024, COLOR_WHITE);
            print_string(" MB)\n", COLOR_WHITE);
        } else {
            print_string("Failed to read disk info\n", COLOR_RED);
        }
        return 0;
    }
    else {
        print_string("Unknown command: ", COLOR_RED);
        print_string(cmd, COLOR_RED);
        print_char('\n', COLOR_RED);
        return 0;
    }
    return 0;
}