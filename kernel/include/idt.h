#ifndef IDT_H
#define IDT_H

#include <stdint.h>

typedef struct {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t  ist;
    uint8_t  attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

void idt_init(void);
void idt_set_gate(uint8_t vector, void* handler, uint16_t selector, uint8_t attributes);

#endif