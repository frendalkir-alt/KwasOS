// kernel/core/isr.c

#include <stdint.h>
#include <video.h>
#include <idt.h>
#include <pic.h>
#include <keyboard.h>
#include <timer.h>

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) registers_t;

void isr_handler(registers_t* regs) {
    if (regs->int_no < 32) {
        print_string("Exception #", COLOR_RED);
        print_int(regs->int_no, COLOR_RED);
        print_string(" occurred. Error code: ", COLOR_RED);
        print_int(regs->err_code, COLOR_RED);
        print_string("\n", COLOR_RED);
        for(;;);
    }
    else if (regs->int_no >= 32 && regs->int_no < 48) {
        switch (regs->int_no) {
            case 32:
                timer_handler();
                break;
            case 33:
                keyboard_handler();
                break;
        }
        pic_eoi(regs->int_no - 32);
    }
    else {
        print_string("Unhandled interrupt ", COLOR_RED);
        print_int(regs->int_no, COLOR_RED);
        print_string("\n", COLOR_RED);
    }
}