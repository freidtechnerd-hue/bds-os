#include "os.h"
#include "storage.h"
void kmain64(u32 magic,u32 info){
    serial_init();serial("BDX-OS x86-64 native long-mode edition\n");
    if(magic!=0x36d76289){serial("FATAL Multiboot2 magic\n");return;}
    int ready=0;u32 total=*(u32*)(usize)info;
    for(u8*p=(u8*)(usize)info+8;p<(u8*)(usize)info+total;){
        u32 n=*(u32*)(p+4);if(n<8)return;
        if(*(u32*)p==6){u32 step=*(u32*)(p+8);if(step<24)return;for(u8*e=p+16;e+24<=p+n;e+=step){unsigned long long start=*(unsigned long long*)e,len=*(unsigned long long*)(e+8);if(*(u32*)(e+16)==1&&start<=0x100000&&start+len>=0xd000000)ready=1;}}
        p+=(n+7)&~7u;
    }
    if(!ready){serial("FATAL: x64 live edition requires 256 MiB RAM\n");return;}
    /* Protect the live module from the fixed app, heap and canvas regions. */
    for(u8*p=(u8*)(usize)info+8;p<(u8*)(usize)info+total;){u32 n=*(u32*)(p+4);if(n<8)return;
        if(*(u32*)p==3){u32 a=*(u32*)(p+8),b=*(u32*)(p+12);if(b<=a||b-a>64*1024*1024)return;extern void*memmove(void*,const void*,usize);memmove((void*)0x7000000,(void*)(usize)a,b-a);*(u32*)(p+8)=0x7000000;*(u32*)(p+12)=0x7000000+b-a;}p+=(n+7)&~7u;
    }
    video_init(info);heap_reset();hardware_init();clear_console();
    print("PASS: 64-bit C, 64-bit pointers, paging and 64-bit IRQ handlers\n");
    storage_init(info);if(fs_init()){print("FATAL: no valid BDSFS app volume\n");return;}
    display_init(0);font_smooth(1);settings_init();wallpaper_init();boot_splash();
    print("Native x64 apps: HELLO CALC EDITOR SYSINFO GAME PUZZLE\n");
    print("Live ISO data is read-only; attach the x64 data image for persistent files.\n");
    print("Preview edition: no BIOS calls, networking, audio or i386 app compatibility.\n");
    if(graphics)desktop(0);else shell();
}
