bits 32
section .multiboot
align 8
header:
    dd 0xe85250d6,0,header_end-header
    dd -(0xe85250d6+(header_end-header))
    dw 5,0
    dd 20,1024,768,32
    align 8
    dw 0,0
    dd 8
header_end:
section .text
global _start
extern kmain64
_start:
    cli
    cld
    mov esp,stack_top
    mov edi,eax
    mov esi,ebx
    mov eax,0x80000000
    cpuid
    cmp eax,0x80000001
    jb unsupported
    mov eax,0x80000001
    cpuid
    test edx,1<<29
    jz unsupported
    mov eax,pdpt
    or eax,3
    mov [pml4],eax
    xor ecx,ecx
.pdpt:
    mov eax,ecx
    shl eax,12
    add eax,pds
    or eax,3
    mov [pdpt+ecx*8],eax
    inc ecx
    cmp ecx,4
    jne .pdpt
    xor ecx,ecx
.pages:
    mov eax,ecx
    shl eax,21
    or eax,0x83
    mov [pds+ecx*8],eax
    inc ecx
    cmp ecx,2048
    jne .pages
    mov eax,cr4
    or eax,1<<5
    mov cr4,eax
    mov eax,pml4
    mov cr3,eax
    mov ecx,0xc0000080
    rdmsr
    or eax,1<<8
    wrmsr
    lgdt [gdt_ptr]
    mov eax,cr0
    or eax,1<<31
    mov cr0,eax
    jmp 8:long_entry
unsupported:
    mov word [0xb8000],0x4f58
    mov word [0xb8002],0x4f36
    mov word [0xb8004],0x4f34
.stop: hlt
    jmp .stop
bits 64
long_entry:
    mov ax,16
    mov ds,ax
    mov es,ax
    mov ss,ax
    xor eax,eax
    mov fs,ax
    mov gs,ax
    mov rsp,stack_top
    call kmain64
.halt: cli
    hlt
    jmp .halt

global load_gdt,irq_timer,fault,call_app,app_exit
extern timer_irq,fault_c
load_gdt: ret
%macro SAVE 0
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
%endmacro
%macro RESTORE 0
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
%endmacro
irq_timer:
    SAVE
    cld
    mov rbx,rsp
    and rsp,-16
    call timer_irq
    mov rsp,rbx
    RESTORE
    iretq
fault:
    push qword 0
    push qword 255
fault_common:
    cli
    cld
    mov rdi,rsp
    and rsp,-16
    call fault_c
    jmp long_entry.halt
%assign vector 0
%rep 32
fault_%+vector:
%if vector != 8 && vector != 10 && vector != 11 && vector != 12 && vector != 13 && vector != 14 && vector != 17 && vector != 21 && vector != 29 && vector != 30
    push qword 0
%endif
    push qword vector
    jmp fault_common
%assign vector vector+1
%endrep
call_app:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov [saved_rsp],rsp
    mov rax,rdi
    mov rdi,rsi
    mov rsi,rdx
    mov rdx,rcx
    sub rsp,8
    call rax
    add rsp,8
.return:
    cld
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
app_exit:
    mov eax,edi
    mov rsp,[saved_rsp]
    jmp call_app.return
section .rodata
align 8
gdt: dq 0,0x00af9a000000ffff,0x00cf92000000ffff
gdt_ptr: dw 23
    dq gdt
global fault_vectors
fault_vectors:
%assign vector 0
%rep 32
    dq fault_%+vector
%assign vector vector+1
%endrep
section .bss
alignb 4096
pml4: resb 4096
pdpt: resb 4096
pds: resb 16384
saved_rsp: resq 1
alignb 16
resb 262144
stack_top:
section .note.GNU-stack noalloc noexec nowrite progbits
