// kernel/core/idt.c

#include <idt.h>
#include <video.h>

extern void isr_0(void);  extern void isr_1(void);  extern void isr_2(void);  extern void isr_3(void);
extern void isr_4(void);  extern void isr_5(void);  extern void isr_6(void);  extern void isr_7(void);
extern void isr_8(void);  extern void isr_9(void);  extern void isr_10(void); extern void isr_11(void);
extern void isr_12(void); extern void isr_13(void); extern void isr_14(void); extern void isr_15(void);
extern void isr_16(void); extern void isr_17(void); extern void isr_18(void); extern void isr_19(void);
extern void isr_20(void); extern void isr_21(void); extern void isr_22(void); extern void isr_23(void);
extern void isr_24(void); extern void isr_25(void); extern void isr_26(void); extern void isr_27(void);
extern void isr_28(void); extern void isr_29(void); extern void isr_30(void); extern void isr_31(void);
extern void isr_32(void); extern void isr_33(void); extern void isr_34(void); extern void isr_35(void);
extern void isr_36(void); extern void isr_37(void); extern void isr_38(void); extern void isr_39(void);
extern void isr_40(void); extern void isr_41(void); extern void isr_42(void); extern void isr_43(void);
extern void isr_44(void); extern void isr_45(void); extern void isr_46(void); extern void isr_47(void);

idt_entry_t idt[256];
idtr_t idtr;

void idt_set_gate(uint8_t vector, void* handler, uint16_t selector, uint8_t attributes) {
    uint64_t addr = (uint64_t)handler;
    idt[vector].isr_low   = addr & 0xFFFF;
    idt[vector].kernel_cs = selector;
    idt[vector].ist       = 0;
    idt[vector].attributes = attributes;
    idt[vector].isr_mid   = (addr >> 16) & 0xFFFF;
    idt[vector].isr_high  = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].reserved  = 0;
}

void idt_init(void) {
    idtr.base = (uint64_t)&idt;
    idtr.limit = sizeof(idt_entry_t) * 256 - 1;

    uint16_t kernel_cs = 0x08;
    uint8_t attrs = 0x8E;

    void* isr_table[] = {
        isr_0,  isr_1,  isr_2,  isr_3,  isr_4,  isr_5,  isr_6,  isr_7,
        isr_8,  isr_9,  isr_10, isr_11, isr_12, isr_13, isr_14, isr_15,
        isr_16, isr_17, isr_18, isr_19, isr_20, isr_21, isr_22, isr_23,
        isr_24, isr_25, isr_26, isr_27, isr_28, isr_29, isr_30, isr_31
    };
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, isr_table[i], kernel_cs, attrs);
    }

    void* irq_table[] = {
        isr_32, isr_33, isr_34, isr_35, isr_36, isr_37, isr_38, isr_39,
        isr_40, isr_41, isr_42, isr_43, isr_44, isr_45, isr_46, isr_47
    };
    for (int i = 0; i < 16; i++) {
        idt_set_gate(32 + i, irq_table[i], kernel_cs, attrs);
    }

    __asm__ volatile("lidt %0" : : "m"(idtr));
}