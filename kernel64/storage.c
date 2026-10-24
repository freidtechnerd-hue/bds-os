#include "os.h"
#include "storage.h"
Drive drives[MAX_DRIVES];Volume volumes[MAX_VOLUMES];int drive_count,volume_count,boot_drive;
static u8*module;static u32 module_blocks,ata_blocks;static int ata;
static int wait_status(int drq){u32 end=ticks()+3000;do{u8 s=inb(0x1f7);if(!s||s==255)return -1;if(!(s&128)){if(s&0x21)return -1;if(!drq||(s&8))return 0;}}while((int)(ticks()-end)<0);return -1;}
static void delay400(void){for(int i=0;i<4;i++)inb(0x3f6);}
static int ata_sector(u32 lba,void*buf,int write){
    if(lba>=ata_blocks||lba>=0x10000000)return -1;
    if(wait_status(0))return -1;outb(0x1f6,0xe0|(lba>>24));delay400();outb(0x1f2,1);outb(0x1f3,lba);outb(0x1f4,lba>>8);outb(0x1f5,lba>>16);outb(0x1f7,write?0x30:0x20);
    if(wait_status(1))return -1;u16*p=buf;for(int i=0;i<256;i++){if(write)outw(0x1f0,p[i]);else p[i]=inw(0x1f0);}delay400();if(wait_status(0))return -1;
    if(write){outb(0x1f7,0xe7);if(wait_status(0))return -1;}return 0;
}
void storage_init(u32 info){
    u32 total=*(u32*)(usize)info;
    for(u8*p=(u8*)(usize)info+8;p<(u8*)(usize)info+total;){u32 n=*(u32*)(p+4);if(n<8)break;if(*(u32*)p==3){u32 a=*(u32*)(p+8),b=*(u32*)(p+12);if(b>a&&(b-a)%512==0){module=(u8*)(usize)a;module_blocks=(b-a)/512;}}p+=(n+7)&~7u;}
    outb(0x3f6,2);outb(0x1f6,0xa0);delay400();outb(0x1f2,0);outb(0x1f3,0);outb(0x1f4,0);outb(0x1f5,0);outb(0x1f7,0xec);
    u16 identify[256];u8 buf[512];
    if(!wait_status(1)){for(int i=0;i<256;i++)identify[i]=inw(0x1f0);ata_blocks=(u32)identify[60]|(u32)identify[61]<<16;
        if(ata_blocks&&ata_blocks<=0x10000000&&!ata_sector(0,buf,0)&&!memcmp(buf,"BDSFS01",8)){u32 n=*(u32*)(buf+8);if(n>=1024&&n<=ata_blocks&&*(u32*)(buf+12)==128)ata=1;}
    }
    if(ata){drives[0]=(Drive){ata_blocks,512,0,0,"ATA PIO x64"};print("STORAGE64: native ATA PIO writable BDSFS\n");}
    else if(module&&module_blocks>=1024&&!memcmp(module,"BDSFS01",8)&&*(u32*)(module+8)==module_blocks){drives[0]=(Drive){module_blocks,512,0,1,"Live ISO"};print("STORAGE64: read-only live module\n");}
    else return;
    drive_count=volume_count=1;volumes[0]=(Volume){0,0,ata?ata_blocks:module_blocks,1};
}
int disk_io(int drive,u32 lba,u32 count,void*buf,int write){
    if(drive||!drive_count||lba>=drives[0].blocks||count>drives[0].blocks-lba||(!ata&&write))return -1;
    if(!ata){memcpy(buf,module+(usize)lba*512,(usize)count*512);return 0;}
    for(u32 i=0;i<count;i++)if(ata_sector(lba+i,(u8*)buf+(usize)i*512,write))return -1;return 0;
}
int volume_io(int v,u32 lba,void*buf,int write){if(v||!volume_count)return -1;return disk_io(0,lba,1,buf,write);}
void storage_list(void){print(ata?"V0: native ATA PIO / BDSFS / writable\n":"V0: live module / BDSFS / read-only\n");}
int media_mount(int v){return -1;}int media_visit(int v){return -1;}int media_read(int f,u32 off,void*b,u32 n){return -1;}
int media_write(const char*p,const void*b,u32 n){return -1;}int media_mkdir(const char*p){return -1;}int media_remove(int f){return -1;}int media_rename(int f,const char*n,u32 p){return -1;}
