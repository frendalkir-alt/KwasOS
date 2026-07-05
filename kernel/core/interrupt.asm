; kernel/core/interrupt.asm – обработчики прерываний (x86-64, NASM)

extern isr_handler

%macro isr_no_error 1
    global isr_%1
    isr_%1:
        cli
        push byte 0
        push byte %1
        jmp isr_common
%endmacro

%macro isr_error 1
    global isr_%1
    isr_%1:
        cli
        push byte %1
        jmp isr_common
%endmacro

isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp
    call isr_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16
    sti
    iretq

isr_no_error 0
isr_no_error 1
isr_no_error 2
isr_no_error 3
isr_no_error 4
isr_no_error 5
isr_no_error 6
isr_no_error 7
isr_error   8
isr_no_error 9
isr_error   10
isr_error   11
isr_error   12
isr_error   13
isr_error   14
isr_no_error 15
isr_no_error 16
isr_error   17
isr_no_error 18
isr_no_error 19
isr_no_error 20
isr_no_error 21
isr_no_error 22
isr_no_error 23
isr_no_error 24
isr_no_error 25
isr_no_error 26
isr_no_error 27
isr_no_error 28
isr_error   29
isr_error   30
isr_no_error 31

%assign i 32
%rep 16
    isr_no_error i
%assign i i+1
%endrep