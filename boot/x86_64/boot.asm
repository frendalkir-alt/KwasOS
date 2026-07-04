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

    ; Включение A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Загрузка ядра (16 секторов в 0x10000)
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov ah, 0x02
    mov al, 16          ; 16 секторов = 8 КБ (пока достаточно, потом увеличим)
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0x00
    int 0x13
    jc error

    ; Переход в защищённый режим (для включения PAE и long mode)
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

    ; Включаем PAE
    mov eax, cr4
    or eax, 1 << 5      ; PAE bit
    mov cr4, eax

    ; Строим временные страничные таблицы (identity mapping первых 2 МБ)
    ; и отображение 0x100000 -> 0x100000 (ядро)
    ; Для простоты используем identity mapping для первых 2 МБ
    ; и таблицу для ядра (0x100000)
    ; Но чтобы не усложнять, загрузим готовые таблицы.
    ; Для начала можно загрузить PML4 и PDP, которые отображают первые 2 МБ.
    ; Я дам упрощённый вариант – используем identity mapping всего 4 ГБ?
    ; Для старта достаточно отобразить первые 2 МБ (это покроет код ядра в 0x100000).
    ; Мы можем сделать identity mapping для всего 4 ГБ через одну PDP.

    ; Создаём страничные таблицы в памяти (вручную)
    ; Для простоты будем использовать готовый шаблон, но здесь я покажу упрощённый
    ; способ: просто загружаем нулевые таблицы и включаем paging,
    ; но для реальной работы нужно правильно настроить.

    ; Однако правильный подход – выделить память под таблицы и заполнить их.
    ; Поскольку у нас нет менеджера памяти, мы можем выделить страницу для PML4 и PDP.
    ; Я приведу рабочий пример ниже (используем адреса, не занятые кодом).

    ; !!! Внимание: для экономии места я дам компактный код, но он должен работать.

    ; Определим адреса таблиц (например, 0x1000 для PML4, 0x2000 для PDP, 0x3000 для PD)
    ; Эти адреса находятся в нижней памяти (до 1 МБ) и не конфликтуют с загрузчиком (0x7C00)
    ; и ядром (0x10000, потом 0x100000).

    ; Очищаем страницы (мы их будем заполнять)
    mov edi, 0x1000
    mov ecx, 0x3000 - 0x1000
    xor eax, eax
    cld
    rep stosb

    ; Настраиваем PML4: указываем на PDP (0x2000) с флагами присутствия и записи
    mov dword [0x1000], 0x2000 | 0x03   ; present + write
    ; Настраиваем PDP: указываем на PD (0x3000) для первых 4 ГБ
    mov dword [0x2000], 0x3000 | 0x03
    ; Настраиваем PD: 512 записей, каждая указывает на 2 МБ страницу
    ; Для identity mapping: каждый entry = (addr << 21) | 0x83 (present, write, 2MB page)
    ; Для простоты заполним первые 2 записи (покрывают 0-4 МБ)
    mov edi, 0x3000
    mov ebx, 0x00000083   ; flags: present, write, 2MB, global?
    mov ecx, 512
.loop_pd:
    mov [edi], ebx
    add ebx, 0x00200000   ; +2 МБ
    add edi, 8
    loop .loop_pd

    ; Загружаем CR3
    mov eax, 0x1000
    mov cr3, eax

    ; Включаем Long Mode (устанавливаем EFER.LME)
    mov ecx, 0xC0000080   ; EFER MSR
    rdmsr
    or eax, 1 << 8        ; LME bit
    wrmsr

    ; Включаем paging (PG bit в CR0)
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; Теперь мы в совместимом режиме, но мы хотим перейти в 64-битный сегмент.
    ; Загружаем 64-битную GDT
    lgdt [gdt64_desc]

    ; Дальний прыжок в 64-битный сегмент (код 0x08)
    jmp 0x08:long_mode

BITS 64
long_mode:
    ; Настройка сегментов (в long mode сегменты используются иначе, но всё равно загружаем)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, 0x90000

    ; Копируем ядро из 0x10000 в 0x100000 (используем 64-битные регистры)
    mov rsi, 0x10000
    mov rdi, 0x100000
    mov rcx, 2048       ; 8192 / 8 = 1024 qwords (но у нас 32-битный код, поэтому 8 КБ)
    cld
    rep movsq

    ; Передаём управление ядру
    call 0x100000

    cli
    hlt
    jmp $

; 32-битная GDT (для защищённого режима)
gdt32_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF   ; код 32-bit
    dq 0x00CF92000000FFFF   ; данные 32-bit
gdt32_end:
gdt32_desc:
    dw gdt32_end - gdt32_start - 1
    dd gdt32_start

; 64-битная GDT (для long mode)
gdt64_start:
    dq 0x0000000000000000   ; null
    dq 0x00209A0000000000   ; код 64-bit (execute/read, long mode)
    dq 0x0020920000000000   ; данные 64-bit
gdt64_end:
gdt64_desc:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start

times 510 - ($ - $$) db 0
dw 0xAA55