cpu 8086
bits 16
org 0x7c00
jmp short start
nop
db 'BDSOS   '
dw 512
db 1
dw 65
db 2
dw 224
dw 2880
db 0xf0
dw 9
dw 18
dw 2
dd 0
dd 0
db 0,0,0x29
dd 0x42445301
db 'BDS LEGACY '
db 'FAT12   '
start:
    cli
    xor ax,ax
    mov ds,ax
    mov ss,ax
    mov sp,0x7c00
    sti
    mov [drive],dl
    mov ax,0x1000
    mov es,ax
    xor bx,bx
    mov si,1
.load:
    mov ax,si
    xor dx,dx
    mov cx,18
    div cx
    inc dl
    mov cl,dl
    xor dx,dx
    mov di,2
    div di
    mov ch,al
    mov dh,dl
    mov dl,[drive]
    mov ax,0x0201
    push bx
    push si
    int 0x13
    pop si
    pop bx
    jc .fail
    add bx,512
    inc si
    cmp si,65
    jb .load
    mov dl,[drive]
    jmp 0x1000:0
.fail:
    mov si,error
.print:
    lodsb
    or al,al
    jz .halt
    mov ah,0x0e
    int 0x10
    jmp .print
.halt:
    hlt
    jmp .halt
drive db 0
error db 'BDS boot: disk read failed',0
times 510-($-$$) db 0
dw 0xaa55
