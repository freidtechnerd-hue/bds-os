#include "os.h"
/* Realtek RTL8139 legacy PCI DMA interface; 64 KiB RX ring, four TX buffers. */
static u16 io;static u32 cursor,txnext;static u8 ring[65536+16+2048] __attribute__((aligned(256)));
static u8 sendbuf[4][2048] __attribute__((aligned(16)));
static u32 pci(u32 a,int off){outl(0xcf8,0x80000000|a|(off&252));return inl(0xcfc);}
static void rx_enable(void){outl(io+0x30,(u32)ring);outw(io+0x38,0xfff0);outl(io+0x44,0xa|(6<<8)|(3<<11)|(4<<13));}
int rtl_init(u8*mac){
    u32 found=0xffffffff;for(int b=0;b<256&&found==0xffffffff;b++)for(int d=0;d<32&&found==0xffffffff;d++)for(int fn=0;fn<8;fn++){u32 a=b<<16|d<<11|fn<<8,v=pci(a,0);if((v&65535)==65535){if(!fn)break;continue;}if(v==0x813910ec){found=a;break;}if(!fn&&!(pci(a,12)&0x800000))break;}
    if(found==0xffffffff)return -1;u32 bar=pci(found,16);if(!(bar&1)||(bar&~3u)>65535)return -1;io=bar&~3u;u16 command=pci(found,4);outl(0xcf8,0x80000000|found|4);outw(0xcfc,command|5);
    outb(io+0x52,0);outb(io+0x37,0x10);u32 end=ticks()+500;while(inb(io+0x37)&0x10){if((int)(ticks()-end)>=0){io=0;return -1;}sleep(1);}
    for(int i=0;i<6;i++)mac[i]=inb(io+i);cursor=txnext=0;memset(ring,0,sizeof(ring));outw(io+0x3c,0);outw(io+0x3e,0xffff);outb(io+0x37,0x0c);rx_enable();outl(io+0x40,(3<<24)|(6<<8));return 0;
}
int rtl_link(void){return io&&!(inb(io+0x58)&4);}
int rtl_send(const void*frame,u32 len){if(!rtl_link()||len>1518)return -1;u32 i=txnext&3;if(!(inl(io+0x10+i*4)&0x2000))return -1;memcpy(sendbuf[i],frame,len);if(len<60){memset(sendbuf[i]+len,0,60-len);len=60;}outl(io+0x20+i*4,(u32)sendbuf[i]);__asm__ volatile("":::"memory");outl(io+0x10+i*4,len|(32<<16));txnext++;return 0;}
int rtl_receive(void*frame){
    if(inb(io+0x37)&1)return 0;u16 flags=*(u16*)(ring+cursor),len=*(u16*)(ring+cursor+2);if(!(flags&1)||len<18||len>1522){outb(io+0x37,4);cursor=0;rx_enable();outb(io+0x37,12);outw(io+0x3e,0xffff);return 0;}
    for(u32 i=0;i<(u32)len-4;i++)((u8*)frame)[i]=ring[(cursor+4+i)&65535];cursor=(cursor+len+7)&~3u;cursor&=65535;outw(io+0x38,(u16)(cursor-16));outw(io+0x3e,0xffff);return len-4;
}
