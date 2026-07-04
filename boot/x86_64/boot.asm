; boot/x64_86/boot.asm - 64'х битный загрузчик

BITS 16
ORG 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    in al, 0x92
    or al, 2
    out 0x92, al

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

    lgdt [gdt32_desc]
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

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov edi, 0x1000
    mov ecx, 0x3000 - 0x1000
    xor eax, eax
    cld
    rep stosb

    mov dword [0x1000], 0x2000 | 0x03
    mov dword [0x2000], 0x3000 | 0x03
    mov edi, 0x3000
    mov ebx, 0x00000083
    mov ecx, 512
.loop_pd:
    mov [edi], ebx
    add ebx, 0x00200000
    add edi, 8
    loop .loop_pd

    mov eax, 0x1000
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    lgdt [gdt64_desc]

    jmp 0x08:long_mode

BITS 64
long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, 0x90000

    mov rsi, 0x10000
    mov rdi, 0x100000
    mov rcx, 2048
    cld
    rep movsq

    call 0x100000

    cli
    hlt
    jmp $

gdt32_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt32_end:
gdt32_desc:
    dw gdt32_end - gdt32_start - 1
    dd gdt32_start

gdt64_start:
    dq 0x0000000000000000
    dq 0x00209A0000000000
    dq 0x0020920000000000
gdt64_end:
gdt64_desc:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start

times 510 - ($ - $$) db 0
dw 0xAA55