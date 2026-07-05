// kernel/include/panic.h

#ifndef PANIC_H
#define PANIC_H

enum panic_code {
    PANIC_GENERAL = 0x01,
    PANIC_EXCEPTION = 0x02,
    PANIC_MEMORY = 0x03,
    PANIC_FILESYSTEM = 0x04,
    PANIC_ATA = 0x05,
    PANIC_USER = 0x06,
    PANIC_DIVIDE_BY_ZERO = 0x07,
    PANIC_PAGE_FAULT = 0x08,
    PANIC_GPF = 0x09,
    PANIC_STACK_OVERFLOW = 0x0A,
    PANIC_INVALID_OPCODE = 0x0B,
    PANIC_DOUBLE_FAULT = 0x0C,
};

void kwas_panic(enum panic_code code, const char* message);

#endif