#include "window.h"
#include "keyboard.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Список возможных путей к шрифтам в Linux (и macOS)
static const char* font_paths[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
    "/usr/share/fonts/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
    "/usr/share/fonts/liberation/LiberationMono-Regular.ttf",
    "/System/Library/Fonts/Menlo.ttc",           // macOS
    "/System/Library/Fonts/Courier New.ttf",    // macOS
    NULL
};

static const char* find_font() {
    for (int i = 0; font_paths[i] != NULL; i++) {
        FILE* f = fopen(font_paths[i], "r");
        if (f) {
            fclose(f);
            return font_paths[i];
        }
    }
    return NULL;
}

int gui_init(GUI_Context* ctx, const char* title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return -1;
    }
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF init failed: %s\n", TTF_GetError());
        return -1;
    }

    ctx->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED, width, height,
                                   SDL_WINDOW_SHOWN);
    if (!ctx->window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return -1;
    }
    ctx->renderer = SDL_CreateRenderer(ctx->window, -1,
                                       SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx->renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        return -1;
    }

    // Поиск шрифта
    const char* font_path = find_font();
    if (!font_path) {
        fprintf(stderr, "Could not find any font. Install DejaVu Sans Mono or Liberation Mono.\n");
        return -1;
    }
    ctx->font = TTF_OpenFont(font_path, 14);
    if (!ctx->font) {
        fprintf(stderr, "Could not load font: %s\n", TTF_GetError());
        return -1;
    }
    TTF_SetFontStyle(ctx->font, TTF_STYLE_NORMAL);

    int advance;
    TTF_GlyphMetrics(ctx->font, 'W', NULL, NULL, NULL, NULL, &advance);
    ctx->char_width = advance;
    ctx->char_height = TTF_FontHeight(ctx->font);
    ctx->term_x = 10;
    ctx->term_y = 10;
    ctx->term_width = TERM_COLS * ctx->char_width;
    ctx->term_height = TERM_ROWS * ctx->char_height;
    ctx->running = true;

    terminal_init(&ctx->term);
    keyboard_init();

    return 0;
}

void gui_close(GUI_Context* ctx) {
    if (ctx->font) TTF_CloseFont(ctx->font);
    if (ctx->renderer) SDL_DestroyRenderer(ctx->renderer);
    if (ctx->window) SDL_DestroyWindow(ctx->window);
    TTF_Quit();
    SDL_Quit();
}

void gui_render(GUI_Context* ctx, i8080* cpu) {
    SDL_Renderer* ren = ctx->renderer;
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);

    SDL_Color fg_color = {255, 255, 255, 255};
    SDL_Rect dst;
    dst.w = ctx->char_width;
    dst.h = ctx->char_height;

    for (int y = 0; y < TERM_ROWS; y++) {
        for (int x = 0; x < TERM_COLS; x++) {
            char ch = ctx->term.chars[y][x];
            if (ch < 32) ch = ' ';
            dst.x = ctx->term_x + x * ctx->char_width;
            dst.y = ctx->term_y + y * ctx->char_height;
            // Рисуем фон (чёрный)
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_RenderFillRect(ren, &dst);
            // Рисуем символ
            char str[2] = {ch, 0};
            SDL_Surface* surf = TTF_RenderUTF8_Blended(ctx->font, str, fg_color);
            if (surf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
                if (tex) {
                    SDL_RenderCopy(ren, tex, NULL, &dst);
                    SDL_DestroyTexture(tex);
                }
                SDL_FreeSurface(surf);
            }
        }
    }

    // Курсор (мигающий)
    if (ctx->term.cursor_visible) {
        int cx = ctx->term.cursor_x;
        int cy = ctx->term.cursor_y;
        SDL_Rect cur_rect = {
            ctx->term_x + cx * ctx->char_width,
            ctx->term_y + cy * ctx->char_height,
            ctx->char_width,
            ctx->char_height
        };
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 100);
        SDL_RenderFillRect(ren, &cur_rect);
    }

    debug_render(ctx, cpu);
    SDL_RenderPresent(ren);
}

void gui_handle_events(GUI_Context* ctx, i8080* cpu) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            ctx->running = false;
        }
        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode key = e.key.keysym.sym;
            if (key == SDLK_F10) {
                // Пошаговый режим: выполнить одну инструкцию
                cpu_step(cpu);
                printf("Step executed, PC=0x%04x\n", cpu->pc);
            }
            if (key == SDLK_F5) {
                // Сброс (пока не реализован)
                printf("Reset requested (not implemented)\n");
            }
            // Передача символов в буфер клавиатуры
            // Для обычных символов
            if (key >= SDLK_SPACE && key <= SDLK_z) {
                char c = (char)key;
                // Учитываем Shift для букв
                if (e.key.keysym.mod & KMOD_SHIFT) {
                    if (c >= 'a' && c <= 'z') c -= 32;
                }
                keyboard_put(c);
            }
            if (key == SDLK_RETURN) {
                keyboard_put('\n');
            }
            if (key == SDLK_BACKSPACE) {
                keyboard_put('\b');
            }
            // Также можно обрабатывать Delete, Escape и др.
        }
    }
}

void gui_set_terminal_output(GUI_Context* ctx, char c) {
    terminal_putchar(&ctx->term, c);
}