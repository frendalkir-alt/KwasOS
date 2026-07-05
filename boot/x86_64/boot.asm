; boot/x64_86/boot.asm 

BITS 16
ORG 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Сохраняем номер диска
    mov [boot_drive], dl

    ; Включить A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Загружаем ядро (читаем 128 секторов, начиная с сектора 1)
    mov ax, 0x1000      ; сегмент буфера (0x1000:0x0000)
    mov es, ax
    xor bx, bx          ; смещение 0

    mov ah, 0x02        ; функция чтения
    mov al, 128         ; количество секторов (64 КБ)
    mov ch, 0           ; цилиндр 0
    mov cl, 2           ; сектор 2 (1-й сектор после загрузчика)
    mov dh, 0           ; головка 0
    mov dl, [boot_drive]; загрузочный диск
    int 0x13
    jc error

    ; Успешно загружено
    mov si, msg_ok
    call print16

    ; Переход в защищённый режим
    lgdt [gdt32_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode

error:
    mov si, msg_error
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

boot_drive: db 0
msg_error: db "ERR",0
msg_ok: db "OK",0

BITS 32
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; Настройка PAE, paging, long mode (как в вашем оригинале)
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

    ; Копируем ядро из 0x10000 в 0x100000
    mov rsi, 0x10000
    mov rdi, 0x100000
    mov rcx, 8192   ; 64 КБ / 8 = 8192 qword
    cld
    rep movsq

    call 0x100000
    cli
    hlt
    jmp $

gdt32_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF   ; код
    dq 0x00CF92000000FFFF   ; данные
gdt32_end:
gdt32_desc:
    dw gdt32_end - gdt32_start - 1
    dd gdt32_start

gdt64_start:
    dq 0x0000000000000000
    dq 0x00209A0000000000   ; код 64
    dq 0x0020920000000000   ; данные
gdt64_end:
gdt64_desc:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start

times 510 - ($ - $$) db 0
dw 0xAA55