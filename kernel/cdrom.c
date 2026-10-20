/* Read-only ATAPI/MMC transport for audio CDs on legacy IDE channels.
   BIOS block I/O remains responsible for ordinary disk/DVD files. */
#include "os.h"
typedef struct {u16 io,control;u8 slave;char model[41];} CdDrive;
static CdDrive cds[4];static int count=-1;
static int wait_status(u16 io,int drq){
    u32 deadline=ticks()+2000;int limit=4000000;
    while(limit--){u8 s=inb(io+7);if(!s||s==255)return -1;if(!(s&128)){if(s&1)return -1;if(!drq||(s&8))return 0;}if((int)(ticks()-deadline)>=0)return -1;}
    return -1;
}
static void select_drive(CdDrive*d){outb(d->io+6,0xa0|(d->slave<<4));for(int i=0;i<4;i++)inb(d->control);}
static int packet(CdDrive*d,const u8*cdb,void*out,u32 cap){
    select_drive(d);if((inb(d->io+7)&128)&&wait_status(d->io,0))return -1;outb(d->control,2);
    outb(d->io+1,0);outb(d->io+4,0xfe);outb(d->io+5,0xff);outb(d->io+7,0xa0);
    if(wait_status(d->io,1)){outb(d->control,0);return -1;}
    if((inb(d->io+2)&3)!=1){outb(d->control,0);return -1;}
    for(int i=0;i<12;i+=2)outw(d->io,cdb[i]|(u16)cdb[i+1]<<8);
    for(int i=0;i<4;i++)inb(d->control);
    u32 done=0;int bad=0;
    for(int phase=0;phase<128;phase++){
        if(wait_status(d->io,0)){bad=1;break;}u8 s=inb(d->io+7);if(!(s&8)){outb(d->control,0);return bad?-1:(int)done;}
        u32 n=inb(d->io+4)|((u32)inb(d->io+5)<<8);if(!n)n=65536;
        if((inb(d->io+2)&3)!=2)bad=1;
        for(u32 i=0;i<n;i+=2){u16 v=inw(d->io);if(!bad&&done+i<cap)((u8*)out)[done+i]=v;if(!bad&&i+1<n&&done+i+1<cap)((u8*)out)[done+i+1]=v>>8;}
        if(n>cap-done){bad=1;done=cap;}else done+=n;
    }
    outb(d->control,0);return -1;
}
int cd_count(void){
    if(count>=0)return count;count=0;
    for(int channel=0;channel<2;channel++)for(int slave=0;slave<2;slave++){
        CdDrive d={channel?0x170:0x1f0,channel?0x376:0x3f6,slave,{0}};select_drive(&d);
        u8 s=inb(d.io+7);if(!s||s==255||(s&128))continue;outb(d.control,2);outb(d.io+7,0xa1);
        if(wait_status(d.io,1)){outb(d.control,0);continue;}u16 words[256];for(int i=0;i<256;i++)words[i]=inw(d.io);outb(d.control,0);
        if((words[0]&0x1f00)!=0x0500)continue;
        for(int i=0;i<20;i++){d.model[i*2]=words[27+i]>>8;d.model[i*2+1]=words[27+i];}for(int i=39;i>=0&&d.model[i]==' ';i--)d.model[i]=0;
        cds[count++]=d;
    }
    return count;
}
static u32 be32(const u8*p){return (u32)p[0]<<24|(u32)p[1]<<16|(u32)p[2]<<8|p[3];}
int cd_info(int id,CdInfo*out){
    if(!out||id<0||id>=cd_count())return -1;u8 cdb[12]={0x43},buf[804];cdb[7]=sizeof(buf)>>8;cdb[8]=sizeof(buf)&255;
    int n=packet(cds+id,cdb,buf,sizeof(buf));if(n<4){/* Unit attention after a disc change: retry once. */n=packet(cds+id,cdb,buf,sizeof(buf));if(n<4)return -1;}
    u32 bytes=((u32)buf[0]<<8|buf[1])+2;if(bytes>(u32)n||bytes<4)return -1;
    memset(out,0,sizeof(*out));strcpy(out->model,cds[id].model);
    for(u32 i=4;i+8<=bytes;i+=8){u8 t=buf[i+2];u32 lba=be32(buf+i+4);if(t==0xaa)out->leadout=lba;else if(t>=1&&t<=99&&out->count<99)out->tracks[out->count++]=(CdTrack){lba,t,buf[i+1]&15,0};}
    return out->count&&out->leadout?0:-1;
}
int cd_read(int id,u32 lba,u32 sectors,void*out){
    if(!out||id<0||id>=cd_count()||!sectors||sectors>16||lba>0xffffff-sectors)return -1;
    u8 cdb[12]={0xbe,4};cdb[2]=lba>>24;cdb[3]=lba>>16;cdb[4]=lba>>8;cdb[5]=lba;cdb[8]=sectors;cdb[9]=0x10;
    return packet(cds+id,cdb,out,sectors*2352)==(int)sectors*2352?(int)sectors:-1;
}
