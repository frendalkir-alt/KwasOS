// drivers/include/timer.h

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

extern volatile uint64_t system_ticks;

void timer_init(void);

void timer_handler(void);

void sleep(uint32_t ms);

uint64_t get_ticks(void);

#endif