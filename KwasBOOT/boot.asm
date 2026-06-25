; KwasBOOT/boot.asm
BITS 16
ORG 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Включение A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Загрузка ядра (16 секторов в 0x10000)
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov ah, 0x02
    mov al, 16
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0x00
    int 0x13
    jc error

    ; GDT
    lgdt [gdt_desc]

    ; Переключение в защищённый режим
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode

error:
    mov si, err_msg
    call print16
    cli
    hlt
    jmp $

print16:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print16
.done:
    ret

err_msg db "ERR",0

BITS 32
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; Копирование ядра из 0x10000 в 0x100000
    mov esi, 0x10000
    mov edi, 0x100000
    mov ecx, 2048
    cld
    rep movsd

    ; Переход к ядру
    call 0x100000
    cli
    hlt
    jmp $

gdt_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end:
gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

times 510 - ($ - $$) db 0
dw 0xAA55