cpu 8086
bits 16
org 0
db 'BDSR'
dw 1,1
dd end-code,0,0,65536,0,0
code:
    mov si,message
.out:
    lodsb
    or al,al
    jz .done
    mov ah,0x0e
    mov bx,7
    int 0x10
    mov bl,al
.serial:
    mov dx,0x3fd
    in al,dx
    test al,0x20
    jz .serial
    mov dx,0x3f8
    mov al,bl
    out dx,al
    jmp .out
.done:
    retf
message db 'Hello from native 8086 machine code!',13,10,0
end:
