#include "os.h"
/* ACPI legacy PM1 control from validated RSDP/RSDT/FADT/DSDT tables. */
static int checksum(const u8*p,u32 n){u8 sum=0;for(u32 i=0;i<n;i++)sum+=p[i];return !sum;}
static u8*table(u32 address,const char*sig){if(address<0x10000||address>0xfff00000)return 0;u8*p=(u8*)address;u32 n=*(u32*)(p+4);return n>=36&&n<=0x100000&&!memcmp(p,sig,4)&&checksum(p,n)?p:0;}
static u8*rsdp(u32 start,u32 end){for(u32 a=start;a+20<=end;a+=16){u8*p=(u8*)a;if(!memcmp(p,"RSD PTR ",8)&&checksum(p,20))return p;}return 0;}
static u8*fadt(void){u32 ebda=(u32)*(volatile u16*)0x40e<<4;u8*p=ebda>=0x80000&&ebda<0xa0000?rsdp(ebda,ebda+1024):0;if(!p)p=rsdp(0xe0000,0x100000);if(!p)return 0;u8*r=table(*(u32*)(p+16),"RSDT");if(!r)return 0;u32 n=*(u32*)(r+4);for(u32 i=36;i+4<=n;i+=4){u8*f=table(*(u32*)(r+i),"FACP");if(f&&*(u32*)(f+4)>=116)return f;}return 0;}
static int integer(u8**p,u8*end){if(*p>=end)return -1;int v=*(*p)++;if(v==0||v==1)return v;if(v==10&&*p<end)return *(*p)++;return -1;}
void system_poweroff(void){
    audio_stop();music_stop();print("POWER: shutting down\n");u8*f=fadt();
    if(f){u32 pa=*(u32*)(f+64),pb=*(u32*)(f+68),smi=*(u32*)(f+48);u8*d=table(*(u32*)(f+40),"DSDT");
        if(d&&pa&&pa<=65535&&pb<=65535){u8*end=d+*(u32*)(d+4);for(u8*p=d+36;p+9<end;p++){
            if(memcmp(p,"_S5_",4)||!(p[-1]==8||(p[-1]=='\\'&&p>=d+38&&p[-2]==8))||p[4]!=0x12)continue;
            u8*q=p+5;int bytes=1+(*q>>6);if(q+bytes+1>=end)continue;q+=bytes;if(*q++<2)continue;int a=integer(&q,end),b=integer(&q,end);if(a<0||a>7||b<0||b>7)continue;
            if(!(inw(pa)&1)&&smi&&smi<=65535&&f[52]){outb(smi,f[52]);u32 until=ticks()+3000;while(!(inw(pa)&1)&&(int)(ticks()-until)<0)sleep(10);}
            if(inw(pa)&1){outw(pa,(inw(pa)&~0x3c00u)|(a<<10)|(1<<13));if(pb)outw(pb,(inw(pb)&~0x3c00u)|(b<<10)|(1<<13));sleep(500);}break;
        }}
    }
    print("POWER: firmware did not power off. You can turn the machine off.\n");extern void console_full(void);console_full();
}
void system_reset(void){
    audio_stop();music_stop();serial("POWER: restarting\n");u8*f=fadt();if(f&&*(u32*)(f+4)>=129&&(f[113]&4)&&f[116]==1&&f[117]==8&&f[118]==0&&*(u32*)(f+124)==0){u32 port=*(u32*)(f+120);if(port&&port<=65535){outb(port,f[128]);sleep(100);}}
    for(int i=0;i<100000&&inb(0x64)&2;i++);outb(0x64,0xfe);sleep(100);outb(0xcf9,6);for(;;)__asm__ volatile("hlt");
}
