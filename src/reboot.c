#include "reboot.h"

void reboot() {
    __asm__ volatile ("cli");
    unsigned char status;
    do {
        __asm__ volatile ("inb $0x64, %0" : "=a"(status));
    } while (status & 0x02);
    __asm__ volatile ("outb %0, $0x64" : : "a"((unsigned char)0xFE));
    while (1) __asm__ volatile ("hlt");
}