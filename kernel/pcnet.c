#include "os.h"
/* AMD Am79C970A/973, 32-bit software style 2, polled descriptor rings. */
typedef struct __attribute__((packed)){u32 address;u16 length,status;u32 message,reserved;}Descriptor;
static volatile Descriptor rx[16] __attribute__((aligned(16))),tx[8] __attribute__((aligned(16)));
static u8 rb[16][2048] __attribute__((aligned(16))),tb[8][2048] __attribute__((aligned(16)));
static struct __attribute__((packed,aligned(16))){u16 mode;u8 rlen,tlen,mac[6];u16 reserved;u32 filter[2],rx,tx;}init;
static u16 io;static int ri,ti;
static u32 pci(u32 a,int o){outl(0xcf8,0x80000000|a|o);return inl(0xcfc);}
static u16 csr(int r){outl(io+0x14,r);return inl(io+0x10);}
static void csw(int r,u16 v){outl(io+0x14,r);outl(io+0x10,v);}
static u16 bcr(int r){outl(io+0x14,r);return inl(io+0x1c);}
static void bcw(int r,u16 v){outl(io+0x14,r);outl(io+0x1c,v);}
int pcnet_init(u8*mac){
    u32 found=0xffffffff;for(int b=0;b<256&&found==0xffffffff;b++)for(int d=0;d<32&&found==0xffffffff;d++)for(int f=0;f<8;f++){u32 a=b<<16|d<<11|f<<8,id=pci(a,0);if((id&65535)==65535){if(!f)break;continue;}if(id==0x20001022){found=a;break;}if(!f&&!(pci(a,12)&0x800000))break;}if(found==0xffffffff)return -1;
    u32 bar=pci(found,16);if(!(bar&1)||(bar&~3u)>65535)return -1;io=bar&~3u;u16 command=pci(found,4);outl(0xcf8,0x80000000|found|4);outw(0xcfc,command|5);
    (void)inw(io+0x14);outl(io+0x10,0);if(csr(0)!=4){io=0;return -1;}bcw(20,2);bcw(2,bcr(2)|2);for(int i=0;i<6;i++)mac[i]=inb(io+i);
    memset(&init,0,sizeof(init));memset((void*)rx,0,sizeof(rx));memset((void*)tx,0,sizeof(tx));init.rlen=4<<4;init.tlen=3<<4;memcpy(init.mac,mac,6);init.rx=(u32)rx;init.tx=(u32)tx;ri=ti=0;
    for(int i=0;i<16;i++){rx[i].address=(u32)rb[i];rx[i].length=(u16)-2048;rx[i].status=0x8000;}for(int i=0;i<8;i++)tx[i].address=(u32)tb[i];
    csw(1,(u32)&init);csw(2,(u32)&init>>16);csw(3,0xffff);csw(0,1);u32 end=ticks()+1000;while(!(csr(0)&0x100)){if((int)(ticks()-end)>=0){csw(0,4);io=0;return -1;}sleep(1);}csw(0,0x102);return 0;
}
int pcnet_link(void){return io&&!!(bcr(4)&0x8000);}
int pcnet_send(const void*data,u32 len){if(!pcnet_link()||len>1518||(tx[ti].status&0x8000))return -1;memcpy(tb[ti],data,len);if(len<60){memset(tb[ti]+len,0,60-len);len=60;}tx[ti].length=(u16)-(int)len;tx[ti].message=0;__asm__ volatile("":::"memory");tx[ti].status=0x8300;ti=(ti+1)&7;csw(0,8);return 0;}
int pcnet_receive(void*data){for(int count=0;count<16;count++){u16 st=rx[ri].status;if(st&0x8000)return 0;u32 len=rx[ri].message&4095;int ok=(st&0x4300)==0x0300&&len>=18&&len<=1522;if(ok)memcpy(data,rb[ri],len-4);rx[ri].message=0;__asm__ volatile("":::"memory");rx[ri].status=0x8000;ri=(ri+1)&15;csw(0,csr(0)&0xff00);if(ok)return len-4;}return 0;}
