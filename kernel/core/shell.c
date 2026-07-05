// kernel/core/shell.c

#include "shell.h"
#include "video.h"
#include "keyboard.h"
#include "string.h"
#include "reboot.h"
#include <timer.h>

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

        if (key == '\n') {
            buffer[pos] = '\0';
            print_char('\n', COLOR_WHITE);
            if (pos > 0) {
                if (history_count == 0 || strcmp(buffer, history[history_count-1]) != 0) {
                    if (history_count < HISTORY_SIZE) {
                        int i;
                        for (i = 0; i <= pos && i < MAX_CMD_LEN-1; i++)
                            history[history_count][i] = buffer[i];
                        history[history_count][pos] = '\0';
                        history_count++;
                    } else {
                        for (int i = 0; i < HISTORY_SIZE-1; i++) {
                            for (int j = 0; j < MAX_CMD_LEN; j++)
                                history[i][j] = history[i+1][j];
                        }
                        for (int i = 0; i <= pos && i < MAX_CMD_LEN-1; i++)
                            history[HISTORY_SIZE-1][i] = buffer[i];
                        history[HISTORY_SIZE-1][pos] = '\0';
                    }
                }
            }
            history_pos = history_count;
            return;
        }
        else if (key == 0x08) {
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
                while (hist_str[i] && i < max_len-1) {
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
                    while (hist_str[i] && i < max_len-1) {
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
        print_string("  help        - show this help\n", COLOR_WHITE);
        print_string("  echo <text> - print text\n", COLOR_WHITE);
        print_string("  ver         - show OS version\n", COLOR_WHITE);
        print_string("  ticks       - show system uptime in ticks (100 Hz)\n", COLOR_WHITE);
        print_string("  cls         - clear the screen\n", COLOR_WHITE);
        print_string("  reboot      - reboot the system\n", COLOR_WHITE);
        print_string("  shutdown    - Turn off the system\n", COLOR_WHITE);
        return 0;
    }
    else if (strcmp(cmd, "cls") == 0) {
        clear_screen();
        print_string("KwasOS 0.1.7\n", COLOR_GREEN);
        return 1;
    }
    else if (strcmp(cmd, "reboot") == 0) {
        print_string("Rebooting...\n", COLOR_RED);
        reboot();
        return 0;
    }
    else if (strcmp(cmd, "ver") == 0) {
        print_string("KwasOS 0.1.7\n", COLOR_GREEN);
        return 0;
    }
    else if (strncmp(cmd, "echo ", 5) == 0) {
        const char *msg = cmd + 5;
        while (*msg == ' ') msg++;
        print_string(msg, COLOR_WHITE);
        print_char('\n', COLOR_WHITE);
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
    if (strcmp(cmd, "shutdown") == 0) {
        print_string("Shutting down...\n", COLOR_WHITE);
        shutdown();
        return 1;
    }
    else {
        print_string("Unknown command: ", COLOR_RED);
        print_string(cmd, COLOR_RED);
        print_char('\n', COLOR_RED);
        return 0;
    }
}