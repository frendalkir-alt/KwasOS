#include "keyboard.h"
#include <string.h>

#define BUFFER_SIZE 128

static uint8_t buffer[BUFFER_SIZE];
static volatile int head = 0;
static volatile int tail = 0;
static volatile int count = 0;

void keyboard_init(void) {
    head = tail = count = 0;
    memset(buffer, 0, BUFFER_SIZE);
}

void keyboard_put(uint8_t key) {
    if (count < BUFFER_SIZE) {
        buffer[tail] = key;
        tail = (tail + 1) % BUFFER_SIZE;
        count++;
    }
}

uint8_t keyboard_read(void) {
    if (count == 0) return 0;
    uint8_t key = buffer[head];
    head = (head + 1) % BUFFER_SIZE;
    count--;
    return key;
}

bool keyboard_has_data(void) {
    return count > 0;
}