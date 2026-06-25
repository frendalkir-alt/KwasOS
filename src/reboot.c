// src/reboot.c
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

// Новая функция для выключения (QEMU/эмуляторы)
void shutdown() {
    __asm__ volatile ("cli");
    // Попытка выключения через порт 0x604 (QEMU, VirtualBox, некоторые ПК)
    __asm__ volatile ("outw %0, $0x604" : : "a"((unsigned short)0x2000));
    // Запасной вариант – порт 0x4004
    __asm__ volatile ("outw %0, $0x4004" : : "a"((unsigned short)0x2000));
    // Если ничего не вышло, уходим в бесконечный HLT
    while (1) __asm__ volatile ("hlt");
}