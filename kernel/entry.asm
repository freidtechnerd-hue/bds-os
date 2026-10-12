bits 32
section .multiboot
align 8
header:
    dd 0xe85250d6,0,header_end-header
    dd -(0xe85250d6+(header_end-header))
    dw 5,0
    dd 20,800,600,32
    align 8
    dw 0,0
    dd 8
header_end:
section .text
global _start
extern kmain
_start:
    cli
    mov esp,stack_top
    push ebx
    push eax
    call kmain
.halt:
    hlt
    jmp .halt
global load_gdt,irq_timer,fault,call_app,app_exit
extern timer_irq,fault_c
load_gdt:
    lgdt [gdt_ptr]
    jmp 8:.reload
.reload:
    mov ax,16
    mov ds,ax
    mov es,ax
    mov fs,ax
    mov gs,ax
    mov ss,ax
    ret
irq_timer:
    pushad
    cld
    call timer_irq
    popad
    iretd
fault:
    cli
    call fault_c
    hlt
call_app:
    push ebp
    push ebx
    push esi
    push edi
    mov [saved_esp],esp
    mov eax,[esp+20]
    push dword [esp+32]
    push dword [esp+32]
    push dword [esp+32]
    call eax
    add esp,12
.return:
    cld
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
app_exit:
    mov eax,[esp+4]
    mov esp,[saved_esp]
    jmp call_app.return
section .rodata
align 8
gdt: dq 0,0x00cf9a000000ffff,0x00cf92000000ffff
gdt_ptr: dw 23
    dd gdt
section .bss
saved_esp: resd 1
alignb 16
resb 65536
stack_top:
section .note.GNU-stack noalloc noexec nowrite progbits
