cpu 8086
bits 16
org 0
start:
    cli
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov sp,0xfffe
    sti
    cld
    mov [drive],dl
    mov dx,0x3fb
    mov al,0x80
    out dx,al
    mov dx,0x3f8
    mov al,1
    out dx,al
    inc dx
    xor al,al
    out dx,al
    mov dx,0x3fb
    mov al,3
    out dx,al
    mov ax,3
    int 0x10
    mov si,banner
    call puts
    mov si,menu
    call puts
.menu:
    call key
    cmp al,'1'
    je .gui
    cmp al,'2'
    je shell
    cmp al,'3'
    je .game
    cmp al,'4'
    je .diag
    cmp al,'5'
    je reboot
    jmp .menu
.gui:
    call desktop16
    jmp shell
.game:
    call game
    jmp shell
.diag:
    call sysinfo
shell:
    mov si,prompt
    call puts
    mov di,line
    xor cx,cx
.input:
    call key
    cmp al,13
    je .enter
    cmp al,10
    je .enter
    cmp al,8
    je .back
    cmp al,32
    jb .input
    cmp cx,120
    jae .input
    stosb
    inc cx
    call putc
    jmp .input
.back:
    jcxz .input
    dec cx
    dec di
    mov al,8
    call putc
    mov al,' '
    call putc
    mov al,8
    call putc
    jmp .input
.enter:
    xor al,al
    stosb
    mov si,newline
    call puts
    mov si,line
    mov di,command
.parse:
    lodsb
    cmp al,' '
    je .args
    or al,al
    jz .empty_args
    cmp al,'a'
    jb .store
    cmp al,'z'
    ja .store
    sub al,32
.store:
    stosb
    jmp .parse
.args:
    cmp byte [si],' '
    jne .args_end
    inc si
    jmp .args
.empty_args:
    dec si
.args_end:
    mov [arg],si
    mov byte [di],0
    mov si,command
    mov di,commands
    xor bx,bx
.find:
    push si
    push di
    call strcmp
    pop di
    pop si
    jz .dispatch
.skip:
    cmp byte [di],0
    je .next
    inc di
    jmp .skip
.next:
    inc di
    inc bx
    cmp byte [di],0
    jne .find
    mov si,unknown
    call puts
    jmp shell
.dispatch:
    shl bx,1
    call [handlers+bx]
    jmp shell
strcmp:
    mov al,[si]
    cmp al,[di]
    jne .done
    inc si
    inc di
    or al,al
    jne strcmp
.done:
    ret
key:
    push dx
.wait:
    mov dx,0x3fd
    in al,dx
    test al,1
    jz .bios
    mov dx,0x3f8
    in al,dx
    pop dx
    ret
.bios:
    mov ah,1
    int 0x16
    jz .wait
    xor ah,ah
    int 0x16
    pop dx
    ret
putc:
    push ax
    push bx
    push dx
    mov ah,0x0e
    xor bh,bh
    int 0x10
    mov bl,al
.wait:
    mov dx,0x3fd
    in al,dx
    test al,0x20
    jz .wait
    mov dx,0x3f8
    mov al,bl
    out dx,al
    pop dx
    pop bx
    pop ax
    ret
puts:
    lodsb
    or al,al
    jz .done
    call putc
    jmp puts
.done:
    ret
help:
    mov si,helptext
    jmp puts
ver:
    mov si,banner
    jmp puts
cls:
    mov ax,3
    int 0x10
    mov ax,0x0600
    mov bh,[theme16]
    xor cx,cx
    mov dx,0x184f
    int 0x10
    xor dx,dx
    xor bh,bh
    mov ah,2
    int 0x10
    ret
; A real 80x25 desktop launcher on the original 8086 instruction set.
desktop16:
    call cls
    mov si,desk_title
    call puts
    mov si,desk_menu
    call puts
.wait:
    call key
    cmp al,'1'
    je .done
    cmp al,27
    je .done
    cmp al,'2'
    je .files
    cmp al,'3'
    je .clock
    cmp al,'4'
    je .game
    cmp al,'5'
    je .settings
    cmp al,'6'
    je reboot
    jmp .wait
.files:
    call cls
    call dir
    jmp .back
.clock:
    call cls
    call date
    call time
    call sysinfo
    jmp .back
.game:
    call game
    jmp desktop16
.settings:
    call settings16
    jmp desktop16
.back:
    mov si,desk_back
    call puts
    call key
    jmp desktop16
.done:
    call cls
    ret
settings16:
    call cls
    mov si,settings_title
    call puts
.wait:
    call key
    cmp al,27
    je .done
    cmp al,'1'
    je .blue
    cmp al,'2'
    je .green
    cmp al,'3'
    je .light
    cmp al,'s'
    je .sound
    cmp al,'S'
    je .sound
    cmp al,'t'
    je .test
    jmp .wait
.blue: mov byte [theme16],0x1f
    jmp settings16
.green: mov byte [theme16],0x0a
    jmp settings16
.light: mov byte [theme16],0x70
    jmp settings16
.sound:
    xor byte [sound16],1
    mov si,sound_changed
    call puts
    jmp .wait
.test:
    call beep16
    jmp .wait
.done: ret
beep16:
    cmp byte [sound16],0
    je .done
    push ax
    push bx
    push cx
    push dx
    mov al,0xb6
    out 0x43,al
    mov ax,2712
    out 0x42,al
    mov al,ah
    out 0x42,al
    in al,0x61
    mov bl,al
    or al,3
    out 0x61,al
    xor ah,ah
    int 0x1a
    mov cx,dx
.wait:
    push cx
    xor ah,ah
    int 0x1a
    pop cx
    sub dx,cx
    cmp dx,2
    jb .wait
    mov al,bl
    out 0x61,al
    pop dx
    pop cx
    pop bx
    pop ax
.done: ret
echo:
    mov si,[arg]
    call puts
    mov si,newline
    jmp puts
sysinfo:
    mov si,infotext
    call puts
    int 0x12
    call number
    mov si,kbtext
    jmp puts
number:
    xor cx,cx
    mov bx,10
.div:
    xor dx,dx
    div bx
    push dx
    inc cx
    or ax,ax
    jne .div
.out:
    pop ax
    add al,'0'
    call putc
    loop .out
    ret
reboot:
    jmp 0xffff:0
date:
    mov ah,4
    int 0x1a
    jc unsupported
    mov al,ch
    call bcd
    mov al,cl
    call bcd
    mov al,'-'
    call putc
    mov al,dh
    call bcd
    mov al,'-'
    call putc
    mov al,dl
    call bcd
    mov si,newline
    jmp puts
time:
    mov ah,2
    int 0x1a
    jc unsupported
    mov al,ch
    call bcd
    mov al,':'
    call putc
    mov al,cl
    call bcd
    mov al,':'
    call putc
    mov al,dh
    call bcd
    mov si,newline
    jmp puts
bcd:
    push cx
    push ax
    mov cl,4
    shr al,cl
    add al,'0'
    call putc
    pop ax
    pop cx
    and al,15
    add al,'0'
    jmp putc
unsupported:
    mov si,notsupported
    jmp puts
pwd:
    mov si,roottext
    jmp puts
; AX=LBA, ES:BX=buffer; 1.44MB floppy geometry only.
read_sector:
    push ax
    push bx
    push cx
    push dx
    push si
    xor dx,dx
    mov cx,18
    div cx
    inc dl
    mov cl,dl
    xor dx,dx
    mov si,2
    div si
    mov ch,al
    mov dh,dl
    mov dl,[drive]
    mov ax,0x0201
    int 0x13
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret
load_root:
    push cs
    pop es
    mov ax,83
    mov bx,rootbuf
    mov cx,14
.read:
    call read_sector
    jc .done
    inc ax
    add bx,512
    loop .read
.done:
    ret
dir:
    call load_root
    jc diskerror
    mov di,rootbuf
    mov bp,224
.entry:
    cmp byte [di],0
    je .done
    cmp byte [di],0xe5
    je .next
    test byte [di+11],0x18
    jnz .next
    mov si,di
    mov cx,8
.name:
    lodsb
    call putc
    loop .name
    mov al,'.'
    call putc
    mov cx,3
.ext:
    lodsb
    call putc
    loop .ext
    mov al,' '
    call putc
    mov ax,[di+28]
    call number
    mov si,newline
    call puts
.next:
    add di,32
    dec bp
    jnz .entry
.done:
    ret
; Find 8.3 name, read FAT chain at segment 2000; limit 60KB.
load_file:
    mov di,fname
    mov cx,11
    mov al,' '
    rep stosb
    mov si,[arg]
    mov di,fname
.name:
    lodsb
    or al,al
    jz .search
    cmp al,'.'
    je .ext
    cmp al,' '
    je .search
    cmp al,'a'
    jb .store
    cmp al,'z'
    ja .store
    sub al,32
.store:
    cmp di,fname+11
    jae .bad
    stosb
    jmp .name
.ext:
    mov di,fname+8
    jmp .name
.search:
    call load_root
    jc .bad
    mov di,rootbuf
    mov bp,224
.entry:
    push di
    mov si,fname
    mov cx,11
    repe cmpsb
    pop di
    je .found
    add di,32
    dec bp
    jnz .entry
.bad:
    stc
    ret
.found:
    cmp word [di+30],0
    jne .bad
    mov ax,[di+28]
    cmp ax,60000
    ja .bad
    mov [filesize],ax
    mov ax,[di+26]
    mov [cluster],ax
    mov bx,fatbuf
    mov ax,65
    mov cx,9
.fat:
    call read_sector
    jc .bad
    inc ax
    add bx,512
    loop .fat
    mov ax,0x2000
    mov es,ax
    xor bx,bx
    mov bp,118
.data:
    mov ax,[cluster]
    cmp ax,2
    jb .restorebad
    cmp ax,2785
    jae .restorebad
    add ax,95
    call read_sector
    jc .restorebad
    add bx,512
    mov ax,[cluster]
    mov si,ax
    shr si,1
    add si,ax
    mov dx,[fatbuf+si]
    test ax,1
    jz .even
    mov cl,4
    shr dx,cl
.even:
    and dx,0x0fff
    mov [cluster],dx
    cmp dx,0xff8
    jae .ok
    dec bp
    jnz .data
.restorebad:
    push cs
    pop es
    stc
    ret
.ok:
    push cs
    pop es
    clc
    ret
type:
    call load_file
    jc diskerror
    push ds
    mov cx,[filesize]
    mov ax,0x2000
    mov ds,ax
    xor si,si
    jcxz .done
.out:
    lodsb
    call putc
    loop .out
.done:
    pop ds
    mov si,newline
    jmp puts
run:
    call load_file
    jc diskerror
    mov ax,0x2000
    mov es,ax
    cmp word [filesize],32
    jb .bad
    cmp word [es:0],0x4442
    jne .bad
    cmp word [es:2],0x5253
    jne .bad
    cmp word [es:4],1
    jne .bad
    cmp word [es:6],1
    jne .bad
    cmp word [es:10],0
    jne .bad
    mov ax,[es:8]
    add ax,32
    jc .bad
    cmp ax,[filesize]
    jne .bad
    cmp word [es:18],0
    jne .bad
    mov ax,[es:16]
    cmp ax,[es:8]
    jae .bad
    add ax,32
    mov [runptr],ax
    push cs
    pop es
    push ds
    push es
    mov ax,0x2000
    mov ds,ax
    call far [cs:runptr]
    pop es
    pop ds
    cld
    mov si,returned
    jmp puts
.bad:
    push cs
    pop es
    mov si,badrun
    jmp puts
diskerror:
    mov si,fileerror
    jmp puts
game:
    xor ax,ax
    int 0x1a
    mov ax,dx
    xor dx,dx
    mov bx,9
    div bx
    add dl,'1'
    mov [answer],dl
    mov si,gametext
    call puts
.wait:
    call key
    cmp al,27
    je .done
    cmp al,'1'
    jb .wait
    cmp al,'9'
    ja .wait
    cmp al,[answer]
    je .win
    cmp al,[answer]
    jb .low
    mov si,lower
    call puts
    jmp .wait
.low:
    mov si,higher
    call puts
    jmp .wait
.win:
    mov si,wintext
    call puts
.done:
    ret
banner db 13,10,'BDX-OS Ultimate | 8086 edition v0.2',13,10,0
menu db '1 Text Desktop  2 Command line  3 Game  4 Diagnostics  5 Reboot',13,10,0
desk_title db '===============================================================================',13,10,' BDX-OS 8086 DESKTOP                 Native BIOS / 80 x 25',13,10,'===============================================================================',13,10,13,10,0
desk_menu db ' [1] Terminal       [2] Files          [3] Clock and system information',13,10,13,10,' [4] Games          [5] Settings       [6] Restart',13,10,13,10,' Select an app by number. ESC returns to Terminal.',13,10,13,10,' Files: FAT12 read-only | Apps: native 8086 .RUN | RAM: conventional memory',13,10,0
desk_back db 13,10,'Press any key to return to the desktop.',13,10,0
settings_title db 'BDX-OS 8086 Settings',13,10,13,10,'1 Blue   2 Green   3 Light',13,10,'S Toggle PC-speaker sound   t Test tone   ESC Back',13,10,13,10,'Settings apply to this session; the floppy filesystem is read-only.',13,10,0
sound_changed db 'Sound setting toggled.',13,10,0
theme16 db 7
sound16 db 1
nogui db 'The 8086 edition uses a text interface; desktop is in the GRUB edition.',13,10,0
prompt db 'BDS:\> ',0
newline db 13,10,0
unknown db 'Unknown command. Use HELP.',13,10,0
helptext db 'HELP VER CLS ECHO DIR TYPE RUN PWD DATE TIME MEM SYSINFO GAME REBOOT',13,10,'DESKTOP SETTINGS BEEP',13,10,'FAT12 reading and native 8086 .RUN supported. File writes are not implemented.',13,10,0
infotext db 'CPU: 8086 instruction set; BIOS text, keyboard, RTC and floppy.',13,10,'Conventional memory: ',0
kbtext db ' KB',13,10,0
roottext db '\',13,10,0
notsupported db 'Unsupported on this edition. Use HELP for implemented commands.',13,10,0
fileerror db 'File missing, too large, invalid FAT chain, or disk read error.',13,10,0
badrun db 'Invalid or incompatible .RUN executable.',13,10,0
returned db 'Program returned to BDX-OS.',13,10,0
gametext db 'Guess the digit 1-9. ESC exits. Enter your guess:',13,10,0
higher db 'Higher!',13,10,0
lower db 'Lower!',13,10,0
wintext db 'Correct! Score: 1',13,10,0
commands db 'HELP',0,'VER',0,'CLS',0,'ECHO',0,'DIR',0,'TYPE',0,'RUN',0,'PWD',0,'DATE',0,'TIME',0,'MEM',0,'SYSINFO',0,'GAME',0,'REBOOT',0,'COPY',0,'MOVE',0,'DEL',0,'MKDIR',0,'RMDIR',0,'REN',0,'EDIT',0,'CD',0,'DOOM',0,'DESKTOP',0,'SETTINGS',0,'BEEP',0,0
handlers dw help,ver,cls,echo,dir,type,run,pwd,date,time,sysinfo,sysinfo,game,reboot,unsupported,unsupported,unsupported,unsupported,unsupported,unsupported,unsupported,unsupported,unsupported,desktop16,settings16,beep16
drive db 0
answer db '7'
arg dw 0
filesize dw 0
cluster dw 0
runptr dw 32,0x2000
fname times 11 db 0
line times 128 db 0
command times 128 db 0
fatbuf times 4608 db 0
rootbuf times 7168 db 0
