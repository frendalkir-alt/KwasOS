#ifndef GUI_WINDOW_H
#define GUI_WINDOW_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "cpu.h"
#include "terminal.h"

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    Terminal term;
    int term_x, term_y;      // координаты верхнего левого угла терминала
    int char_width, char_height;
    int term_width, term_height;
    bool running;
    int scale;               // масштаб символов
} GUI_Context;

int gui_init(GUI_Context* ctx, const char* title, int width, int height);
void gui_close(GUI_Context* ctx);
void gui_render(GUI_Context* ctx, i8080* cpu);
void gui_handle_events(GUI_Context* ctx, i8080* cpu);
void gui_set_terminal_output(GUI_Context* ctx, char c);

#endif