; Relocatable thunk copied to 0x6000. Firmware owns IVT/BDA and handles
; boot-time USB/SATA/optical discovery. No paging or concurrent callers.
bits 32
section .text
global bios_thunk_start,bios_thunk_end
%define LOW(x) (0x6000+x-bios_thunk_start)
bios_thunk_start:
    pushfd
    cli
    pushad
    mov [0x7100],esp
    sgdt [0x7110]
    sidt [0x7118]
    ; BIOS IRQ vectors, timer and cascaded disk IRQs only.
    mov al,0x11
    out 0x20,al
    out 0xa0,al
    mov al,8
    out 0x21,al
    mov al,0x70
    out 0xa1,al
    mov al,4
    out 0x21,al
    mov al,2
    out 0xa1,al
    mov al,1
    out 0x21,al
    out 0xa1,al
    mov al,0xfa
    out 0x21,al
    mov al,0x3f
    out 0xa1,al
    lgdt [LOW(gdtptr)]
    jmp 0x18:LOW(pm16)
bits 16
pm16:
    mov ax,0x20
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov eax,cr0
    and eax,0xfffffffe
    mov cr0,eax
    jmp 0:LOW(real)
real:
    xor ax,ax
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov sp,0x7b00
    lidt [LOW(realidt)]
    mov ax,[0x7000]
    mov dx,[0x7002]
    mov si,0x7200
    sti
    int 0x13
    cli
    pushf
    pop bx
    mov cx,ax
    ; Firmware may have changed segments and GDTR.
    xor ax,ax
    mov ds,ax
    mov [0x7004],bx
    mov [0x7006],cx
    lgdt [LOW(gdtptr)]
    mov eax,cr0
    or eax,1
    mov cr0,eax
    jmp dword 8:LOW(back)
bits 32
back:
    mov ax,16
    mov ds,ax
    mov es,ax
    mov fs,ax
    mov gs,ax
    mov ss,ax
    mov esp,[0x7100]
    lgdt [0x7110]
    lidt [0x7118]
    mov al,0x11
    out 0x20,al
    out 0xa0,al
    mov al,32
    out 0x21,al
    mov al,40
    out 0xa1,al
    mov al,4
    out 0x21,al
    mov al,2
    out 0xa1,al
    mov al,1
    out 0x21,al
    out 0xa1,al
    mov al,0xfe
    out 0x21,al
    mov al,0xff
    out 0xa1,al
    cld
    popad
    popfd
    ret
align 8
gdt: dq 0,0x00cf9a000000ffff,0x00cf92000000ffff
     dq 0x00009a000000ffff,0x000092000000ffff
gdtptr: dw 39
        dd LOW(gdt)
realidt: dw 0x3ff
         dd 0
bios_thunk_end:
section .note.GNU-stack noalloc noexec nowrite progbits
