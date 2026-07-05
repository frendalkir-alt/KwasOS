// drivers/timer.c

#include <timer.h>
#include <io.h>
#include <video.h>

#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND       0x43

#define PIT_FREQUENCY 1193182

#define TARGET_FREQ 100

volatile uint64_t system_ticks = 0;

void timer_init(void) {
    uint16_t divisor = PIT_FREQUENCY / TARGET_FREQ;

    outb(PIT_COMMAND, 0x36);

    outb(PIT_CHANNEL0_DATA, divisor & 0xFF);

    outb(PIT_CHANNEL0_DATA, (divisor >> 8) & 0xFF);

}

void timer_handler(void) {
    system_ticks++;
}

uint64_t get_ticks(void) {
    return system_ticks;
}

void sleep(uint32_t ms) {
    uint64_t ticks_to_wait = (uint64_t)ms * TARGET_FREQ / 1000;
    uint64_t target = system_ticks + ticks_to_wait;

    if (target < system_ticks) {
        while (system_ticks > target) {
            __asm__ volatile ("hlt");
        }
    }

    while (system_ticks < target) {
        __asm__ volatile ("hlt");
    }
}