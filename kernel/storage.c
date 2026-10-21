#include "os.h"
#include "storage.h"
Drive drives[MAX_DRIVES];int drive_count,boot_drive=-1;
Volume volumes[MAX_VOLUMES];int volume_count;
extern u8 bios_thunk_start[],bios_thunk_end[];
static int bios_available;
static int firmware(u16 command,u8 drive){
    volatile u16*r=(u16*)0x7000;r[0]=command;r[1]=drive;r[2]=1;r[4]=0;
    ((void(*)(void))0x6000)();return r[2]&1?-1:0;
}
/* Shared real-mode thunk; no concurrent callers. Video buffer is 0x7400. */
int bios_video(u16 command,u16 bx,u16 cx,u16*result){
    if(!bios_available)return -1;volatile u16*r=(u16*)0x7000;
    r[0]=command;r[1]=0;r[2]=1;r[4]=0x10;r[5]=bx;r[6]=cx;
    ((void(*)(void))0x6000)();if(result)*result=r[7];return r[3]==0x004f?0:-1;
}
int bios_edid(u16 block){
    if(!bios_available)return -1;volatile u16*r=(u16*)0x7000;
    r[0]=0x4f15;r[1]=block;r[2]=1;r[4]=0x10;r[5]=1;r[6]=0;
    ((void(*)(void))0x6000)();return r[3]==0x004f?0:-1;
}
int disk_io(int id,u32 lba,u32 count,void*buf,int write){
    if(id<0||id>=drive_count||!count)return -1;Drive*d=drives+id;
    if(!bios_available||lba>=d->blocks||count>d->blocks-lba||(write&&d->readonly))return -1;
    while(count){u32 n=32768/d->sector_size;if(n>count)n=count;
        u8*p=(u8*)0x7200;memset(p,0,16);p[0]=16;*(u16*)(p+2)=n;*(u16*)(p+6)=0x1000;*(u32*)(p+8)=lba;
        if(write)memcpy((void*)0x10000,buf,n*d->sector_size);
        if(firmware(write?0x4300:0x4200,d->bios))return -1;
        if(!write)memcpy(buf,(void*)0x10000,n*d->sector_size);
        buf=(u8*)buf+n*d->sector_size;lba+=n;count-=n;
    }return 0;
}
int volume_io(int id,u32 lba,void*buf,int write){
    if(id<0||id>=volume_count||lba>=volumes[id].blocks)return -1;
    Volume*v=volumes+id;return disk_io(v->drive,v->start+lba,1,buf,write);
}
static void probe_volume(int drive,u32 start,u32 count){
    if(volume_count>=MAX_VOLUMES)return;u8 b[2048];Drive*d=drives+drive;
    if(start>=d->blocks||!count||count>d->blocks-start||disk_io(drive,start,1,b,0))return;
    int type=0;
    if(d->sector_size==512){
        if(!memcmp(b,"BDSFS01",8)&&*(u32*)(b+8)<=count)type=1;
        else if(b[510]==0x55&&b[511]==0xaa&&*(u16*)(b+11)==512&&b[13]&&!(b[13]&(b[13]-1))&&b[16]>=1&&b[16]<=2&&*(u16*)(b+14))type=2;
    }else if(d->sector_size==2048&&count>16&&!disk_io(drive,start+16,1,b,0)&&b[0]==1&&!memcmp(b+1,"CD001",5))type=3;
    if(type)volumes[volume_count++]=(Volume){drive,start,count,type};
}
void storage_init(u32 info){
    drive_count=volume_count=0;int efi=0;u32 total=*(u32*)info;
    for(u8*p=(u8*)info+8;p<(u8*)info+total;){u32 s=*(u32*)(p+4);if(s<8)break;u32 t=*(u32*)p;if(t==11||t==12)efi=1;if(t==5)boot_drive=*(u32*)(p+8);p+=(s+7)&~7u;}
    /* The installed kernel is loaded from GRUB's memdisk, so Multiboot's
       boot-device tag may name that memdisk. Preserve GRUB's physical origin. */
    for(u8*p=(u8*)info+8;p<(u8*)info+total;){u32 s=*(u32*)(p+4);if(s<8)break;if(*(u32*)p==1){char*q=(char*)p+8;for(u32 j=0;j+10<s-8;j++)if(!memcmp(q+j,"origin=(hd",10)){int n=0,k=j+10,has=0;while(q[k]>='0'&&q[k]<='9'&&n<100){n=n*10+q[k++]-'0';has=1;}if(has&&n<16&&(q[k]==')'||q[k]==','))boot_drive=0x80+n;}}p+=(s+7)&~7u;}
    if(efi){print("Storage: BIOS services unavailable under UEFI. Use legacy/CSM boot.\n");return;}
    if(boot_drive<0&&*(volatile u32*)0x5000==0x44534442){u8 b=*(volatile u8*)0x5004;if(b>=0x80&&b<0x90)boot_drive=b;}
    memcpy((void*)0x6000,bios_thunk_start,bios_thunk_end-bios_thunk_start);bios_available=1;
    for(int b=0x80;b<0xf0&&drive_count<MAX_DRIVES;b++){
        /* BIOS hard disk range and El Torito optical range. */
        if(b==0x90)b=0xe0;
        u8*p=(u8*)0x7200;memset(p,0,74);*(u16*)p=74;
        if(firmware(0x4800,b))continue;
        u32 lo=*(u32*)(p+16),hi=*(u32*)(p+20);u16 sector=*(u16*)(p+24);
        if((sector!=512&&sector!=2048)||(!lo&&!hi))continue;
        Drive*d=drives+drive_count;d->bios=b;d->sector_size=sector;d->blocks=hi?0xffffffff:lo;d->readonly=sector==2048;
        strcpy(d->interface,sector==2048?"CD/DVD":"BIOS disk");
        if(*(u16*)p>=66&&*(u16*)(p+30)==0xbedd){memcpy(d->interface,p+40,8);d->interface[8]=0;}
        int id=drive_count++;u8 mbr[2048];
        if(disk_io(id,0,1,mbr,0))continue;
        int before=volume_count;probe_volume(id,0,d->blocks);
        if(sector==512&&before==volume_count&&mbr[510]==0x55&&mbr[511]==0xaa)
            for(int j=0;j<4;j++){u8*q=mbr+446+j*16;if(q[4]&&q[4]!=0xee)probe_volume(id,*(u32*)(q+8),*(u32*)(q+12));}
    }
    storage_list();
}
static void val(u32 n){char s[16];number(s,n);print(s);}
void storage_list(void){
    print("Detected storage (BIOS firmware):\n");
    for(int i=0;i<drive_count;i++){Drive*d=drives+i;print("DISK ");val(i);print(" | ");print(d->interface);print(" | ");if(d->blocks==0xffffffff)print("capacity unavailable");else{val(d->blocks/(1048576/d->sector_size));print(" MiB");}if(d->bios==boot_drive)print(" | boot media");print(d->readonly?" | read-only\n":" | writable\n");}
    for(int i=0;i<volume_count;i++){Volume*v=volumes+i;print("VOLUME ");val(i);print(" | disk ");val(v->drive);print(v->type==1?" | BDSFS read/write\n":v->type==2?" | FAT read/write\n":" | ISO9660 read-only\n");}
}
