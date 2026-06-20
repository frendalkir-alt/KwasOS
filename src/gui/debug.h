#ifndef DEBUG_H
#define DEBUG_H

#include "window.h"
#include "cpu.h"

// Отрисовка панели регистров и памяти
void debug_render(GUI_Context* ctx, i8080* cpu);

#endif