#include "os.h"
#include "storage.h"
static int memory_ready(u32 info,u32 required){u32 total=*(u32*)info;for(u8*p=(u8*)info+8;p<(u8*)info+total;){u32 size=*(u32*)(p+4);if(size<8)break;if(*(u32*)p==6){u32 step=*(u32*)(p+8);if(step<24)return 0;for(u8*e=p+16;e+24<=p+size;e+=step){unsigned long long start=*(unsigned long long*)e,len=*(unsigned long long*)(e+8);if(*(u32*)(e+16)==1&&start<=0x100000&&len>=required-start)return 1;}}p+=(size+7)&~7u;}return 0;}
void kmain(u32 magic,u32 info){
    serial_init();serial("BDX-OS Ultimate | GRUB i386 edition v0.2\n");
    if(magic!=0x36d76289){serial("FATAL Multiboot2 magic\n");return;}
    if(!memory_ready(info,0x7000000)){serial("FATAL: need contiguous RAM through 112 MiB; use QEMU -m 128\n");return;}
    video_init(info);clear_console();
    /* Move install module clear of the app/heap slots before allocating. */
    u32 itotal=*(u32*)info;for(u8*p=(u8*)info+8;p<(u8*)info+itotal;){u32 s=*(u32*)(p+4);if(s<8)break;if(*(u32*)p==3){u32 a=*(u32*)(p+8),b=*(u32*)(p+12);if(b>a&&b-a<=88*1024*1024){if(!memory_ready(info,0x7000000+b-a)){print("FATAL: installer requires 256 MiB RAM\nNo disks changed. Reboot with more RAM.\n");extern void console_full(void);console_full();return;}memcpy((void*)0x7000000,(void*)a,b-a);*(u32*)(p+8)=0x7000000;*(u32*)(p+12)=0x7000000+b-a;}}p+=(s+7)&~7u;}
    heap_reset();hardware_init();clear_console();
    print("PASS: Multiboot2 protected-mode entry\n");
    if(!graphics)print("Framebuffer unavailable: text console fallback\n");else print("PASS: RGB framebuffer\n");
    storage_init(info);
    if(fs_init())print("No supported data volume found. Use DRIVES to inspect storage.\n");else print(volumes[fs_volume()].type==1?"PASS: BDSFS mounted and validated\n":volumes[fs_volume()].type==2?"PASS: FAT mounted\n":"PASS: ISO9660 mounted read-only\n");
    char mode[32]="desktop";u32 total=*(u32*)info;for(u8*p=(u8*)info+8;p<(u8*)info+total;){u32 size=*(u32*)(p+4);if(size<8)break;if(*(u32*)p==1){int j=0;char*q=(char*)p+8;while(q[j]&&q[j]!=' '&&j<31){mode[j]=q[j];j++;}mode[j]=0;}p+=(size+7)&~7u;}
    display_init(!strcmp(mode,"desktop")||!strcmp(mode,"games"));
    audio_init();sfx_init();
    net_init();
    print("BDS boot checkpoint ready\n");
    if(!strcmp(mode,"installer")){installer(info);shell();}
    else if(!strcmp(mode,"diagnostics")){char cmd[]="SYSINFO";shell_command(cmd);shell();}
    else if(!strcmp(mode,"shell")||!graphics)shell();else{font_smooth(1);settings_init();wallpaper_init();boot_splash();desktop(!strcmp(mode,"games")?4:0);}
    for(;;)__asm__ volatile("hlt");
}
