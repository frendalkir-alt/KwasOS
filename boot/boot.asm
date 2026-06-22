; boot/boot.asm - мультизагрузочный заголовок и переход на C-код
section .multiboot
align 4
    dd 0x1BADB002          ; магическое число Multiboot
    dd 0x00                ; флаги (0 = базовый режим)
    dd -(0x1BADB002 + 0x00) ; контрольная сумма

    ; Запрос видеорежима (текстовый 80x25)
    dd 0                   ; mode type: 0 = линейный графический, но мы хотим текст?
    dd 0                   ; width
    dd 0                   ; height
    dd 0                   ; depth

section .text
global start
extern kmain

start:
    ; Устанавливаем стек (16 КБ)
    mov esp, stack_top

    ; Вызываем kmain(unsigned int magic, unsigned int addr)
    push ebx               ; указатель на структуру Multiboot
    push eax               ; магическое число
    call kmain

    ; Бесконечный цикл
    cli
    hlt
    jmp $

section .bss
align 16
stack_bottom:
    resb 16384
stack_top: