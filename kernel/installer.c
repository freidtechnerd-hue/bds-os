#include "os.h"
#include "storage.h"
extern void console_full(void);
static void val(u32 v){char n[16];number(n,v);print(n);}
static u32 crc32(const u8*p,u32 n){u32 table[256];for(u32 i=0;i<256;i++){u32 c=i;for(int j=0;j<8;j++)c=(c>>1)^((c&1)?0xedb88320:0);table[i]=c;}u32 c=0xffffffff;while(n--)c=table[(c^*p++)&255]^(c>>8);return c^0xffffffff;}
static void line(char*out,int max){int n=0;for(;;){console_full();int k=key(1);if(k==27){out[0]=0;print("\nCancelled.\n");return;}if(k==13){out[n]=0;print("\n");return;}if(k==8&&n){n--;print("\b");}else if(k>=32&&k<127&&n<max-1){out[n++]=k;char s[2]={k,0};print(s);}}}
void installer(u32 info){
    u8*image=0;u32 bytes=0,total=*(u32*)info;
    for(u8*p=(u8*)info+8;p<(u8*)info+total;){u32 n=*(u32*)(p+4);if(n<8)break;if(*(u32*)p==3){image=(u8*)*(u32*)(p+8);bytes=*(u32*)(p+12)-(u32)image;}p+=(n+7)&~7u;}
    clear_console();print("BDS-OS DISK INSTALLER (legacy BIOS)\n\n");
    if(!image||bytes<512||bytes%512||image[510]!=0x55||image[511]!=0xaa||memcmp(image+432,"BDSINST1",8)){print("Installer payload missing or invalid. No disks changed.\n");return;}
    print("Checking installer payload CRC32...\n");console_full();
    u32 expected_crc=*(u32*)(image+440);*(u32*)(image+440)=0;
    u32 actual_crc=crc32(image,bytes);*(u32*)(image+440)=expected_crc;
    if(actual_crc!=expected_crc){print("Installer payload checksum failed. No disks changed.\n");return;}
    u32 sectors=bytes/512,ds=*(u32*)(image+446+16+8);if(ds>=sectors||memcmp(image+ds*512,"BDSFS01",8)){print("Invalid data partition. No disks changed.\n");return;}
    storage_list();print("\nThis installer replaces the selected disk's partition table\nand ALL its existing data. It installs GRUB, the kernel,\napps, and BDSFS. The installed OS needs no CD or USB.\n\nMinimum target: ");val(bytes/(1024*1024));print(" MiB. Minimum RAM: 256 MiB.\nBoot media and optical disks cannot be selected.\nPress ESC to cancel without writing anything.\n\nTarget disk number: ");
    char input[64];line(input,sizeof(input));if(!input[0])return;for(char*p=input;*p;p++)if(*p<'0'||*p>'9'){print("Invalid disk number. No disks changed.\n");return;}
    int id=atoi(input);if(id<0||id>=drive_count||drives[id].readonly||drives[id].bios==boot_drive||drives[id].sector_size!=512||drives[id].blocks<sectors){print("Target is unavailable, too small, read-only, or the boot disk.\nNo disks changed.\n");return;}
    u8 b[32768];if(disk_io(id,0,1,b,0)){print("Target read failed. No disks changed.\n");return;}
    for(int i=0;i<4;i++)if(b[446+i*16+4]==0xee){print("GPT target detected. This MBR installer refuses GPT disks.\nUse a blank dedicated disk. No disks changed.\n");return;}
    print("\nSelected DISK ");val(id);print(" | ");print(drives[id].interface);print(" | ");val(drives[id].blocks/2048);print(" MiB\nTo permanently erase it, type exactly: ERASE ");val(id);print("\nConfirmation: ");line(input,sizeof(input));char expected[32]="ERASE ";number(expected+6,id);if(strcmp(input,expected)){print("Confirmation did not match. No disks changed.\n");return;}
    print("Installing and verifying every written block...\n");console_full();
    /* Sector zero is committed last. Expand the BDSFS partition within the
       current format's 8 GiB ceiling; firmware sector addressing is 32-bit. */
    u32 capacity=drives[id].blocks-ds;if(capacity>0x1000000)capacity=0x1000000;
    *(u32*)(image+446+16+12)=capacity;*(u32*)(image+ds*512+8)=capacity;
    for(u32 s=1;s<sectors;){u32 n=64;if(n>sectors-s)n=sectors-s;
        if(disk_io(id,s,n,image+s*512,1)||disk_io(id,s,n,b,0)||memcmp(b,image+s*512,n*512)){print("INSTALL FAILED: disk write/read-back mismatch.\nDo not boot this incomplete installation.\n");return;}s+=n;
        if(s%16384<64){print("Verified ");val(s*512/(1024*1024));print(" MiB\n");console_full();}
    }
    if(disk_io(id,0,1,image,1)||disk_io(id,0,1,b,0)||memcmp(b,image,512)){print("INSTALL FAILED: boot-sector verification failed.\n");return;}
    print("INSTALL COMPLETE: bootloader, kernel, apps and data verified.\nRemove the installer USB/CD/DVD, then reboot from the disk.\nPress Enter to reboot, or ESC to inspect the shell.\n");console_full();
    int k;do{k=key(1);}while(k!=13&&k!=27);if(k==13){while(inb(0x64)&2);outb(0x64,0xfe);for(;;)__asm__ volatile("hlt");}
    storage_init(info);fs_init();
}
