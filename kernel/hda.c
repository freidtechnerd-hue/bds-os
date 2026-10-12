/* Original BDS HDA PCM output: CORB/RIRB codec discovery and an analog DAC path. */
#include "os.h"
static volatile u8*regs;static int stream,active,fill,codec,dac,pin;
static u32 corb[256] __attribute__((aligned(128)));
static volatile u32 rirb[256][2] __attribute__((aligned(128)));
static u32 bdl[8][4] __attribute__((aligned(128)));
static short buffers[8][1024] __attribute__((aligned(128)));
static int cwp,rrp;static u32 group_out_amp,group_in_amp;
typedef struct{u32 caps;u8 next[32],count,choice,visited;}Widget;
static Widget widgets[256];
extern void audio_fill(short*,u32);
static u8 r8(int n){return regs[n];}static u16 r16(int n){return *(volatile u16*)(regs+n);}static u32 r32(int n){return *(volatile u32*)(regs+n);}
static void w8(int n,u8 v){regs[n]=v;}static void w16(int n,u16 v){*(volatile u16*)(regs+n)=v;}static void w32(int n,u32 v){*(volatile u32*)(regs+n)=v;}
static int wait_bits(int n,u32 mask,int set){u32 until=ticks()+200;while(!!(r32(n)&mask)!=set){if((int)(until-ticks())<=0)return -1;sleep(1);}return 0;}
static u32 verb(int node,u32 command){
    w8(0x5d,5);int index=(cwp+1)&255;corb[index]=codec<<28|node<<20|command;__asm__ volatile("":::"memory");cwp=index;w16(0x48,cwp);
    u32 until=ticks()+100;while((r16(0x58)&255)==rrp){if((int)(until-ticks())<=0)return 0xffffffff;sleep(1);}
    while((r16(0x58)&255)!=rrp){rrp=(rrp+1)&255;u32 extra=rirb[rrp][1];if(!(extra&0x10)&&(extra&15)==(u32)codec)return rirb[rrp][0];}return 0xffffffff;
}
static u32 parameter(int node,int id){return verb(node,0xf0000|id);}
static int route(int node,int depth){
    if(node<1||node>255||depth>16||widgets[node].visited)return -1;Widget*w=widgets+node;w->visited=1;int type=w->caps>>20&15;
    if(type==0&&!(w->caps&0x200)){dac=node;return 0;}if(type!=2&&type!=3&&type!=4)return -1;
    for(int i=0;i<w->count;i++)if(!route(w->next[i],depth+1)){w->choice=i;return 0;}return -1;
}
static void enable_path(int node,int depth){
    if(depth>16||node<1)return;Widget*w=widgets+node;if(w->caps&0x400)verb(node,0x70500);int type=w->caps>>20&15;
    if(w->caps&4){u32 cap=w->caps&8?parameter(node,0x12):group_out_amp;if(cap!=0xffffffff)verb(node,0x30000|0xb000|(cap&127));}
    if(w->caps&2){u32 cap=w->caps&8?parameter(node,0x0d):group_in_amp;if(cap!=0xffffffff)verb(node,0x30000|0x7000|w->choice<<8|(cap&127));}
    if(type==0){verb(node,0x20000|0x11);verb(node,0x70600|0x10);return;}
    if(w->count){if(type!=2)verb(node,0x70100|w->choice);enable_path(w->next[w->choice],depth+1);}
}
static int discover(void){
    u32 groups=parameter(0,4);if(groups==0xffffffff)return -1;int start=groups>>16&255,count=groups&255;
    for(int group=start;group<start+count&&group<256;group++){
        if((parameter(group,5)&255)!=1)continue;verb(group,0x70500);group_out_amp=parameter(group,0x12);group_in_amp=parameter(group,0x0d);u32 children=parameter(group,4);int first=children>>16&255,last=first+(children&255);if(last>256)last=256;memset(widgets,0,sizeof(widgets));
        for(int n=first;n<last;n++){Widget*w=widgets+n;w->caps=parameter(n,9);u32 cl=parameter(n,0x0e);if(cl==0xffffffff)continue;int len=cl&127,longform=cl&128,prev=0;
            for(int i=0;i<len&&w->count<32;){u32 list=verb(n,0xf0200|i);int num=longform?2:4;for(int j=0;j<num&&i<len;j++,i++){int raw=longform?(list>>(j*16))&65535:(list>>(j*8))&255,range=raw&(longform?0x8000:0x80),to=raw&(longform?0x7fff:0x7f);if(to>255)continue;if(range){for(int k=prev+1;k<=to&&w->count<32;k++)w->next[w->count++]=k;}else if(w->count<32)w->next[w->count++]=to;prev=to;}}}
        for(int pass=0;pass<3;pass++)for(int n=first;n<last;n++){Widget*w=widgets+n;if((w->caps>>20&15)!=4||(w->caps&0x200)||!(parameter(n,0x0c)&16))continue;u32 config=verb(n,0xf1c00);int device=config>>20&15;if(config==0xffffffff||(config>>30)==1||device!=(pass==0?1:pass==1?0:2))continue;for(int i=0;i<256;i++)widgets[i].visited=0;if(!route(n,0)){pin=n;enable_path(pin,0);verb(pin,0x70700|(device==2?0xc0:0x40));verb(pin,0x70c00|2);return 0;}}
    }return -1;
}
static u32 pci(u32 a,int n){outl(0xcf8,0x80000000|a|n);return inl(0xcfc);}
static int controller(u32 address){
    u32 bar=pci(address,0x10);if((bar&1)||!(bar&~15u)||((bar&6)==4&&pci(address,0x14)))return -1;regs=(volatile u8*)(bar&~15u);
    u16 command=pci(address,4);outl(0xcf8,0x80000000|address|4);outw(0xcfc,command|6);
    w32(0x20,0);w8(0x4c,0);w8(0x5c,0);w32(8,r32(8)&~1u);if(wait_bits(8,1,0))return -1;w32(8,r32(8)|1);if(wait_bits(8,1,1))return -1;sleep(10);
    u16 caps=r16(0);if(!(caps>>12))return -1;stream=0x80+((caps>>8)&15)*32;
    if(!(r8(0x4e)&0x40)||!(r8(0x5e)&0x40))return -1;w8(0x4e,2);w8(0x5e,2);memset(corb,0,sizeof(corb));memset((void*)rirb,0,sizeof(rirb));
    w32(0x40,(u32)corb);w32(0x44,0);w16(0x48,0);w16(0x4a,0x8000);w16(0x4a,0);cwp=0;
    /* Enable response status, with controller interrupts disabled; poll/ack it. */
    w32(0x50,(u32)rirb);w32(0x54,0);w16(0x58,0x8000);rrp=0;w16(0x5a,1);w8(0x5c,3);w8(0x4c,2);
    u16 present=r16(0x0e);for(codec=0;codec<15;codec++)if((present&(1<<codec))&&!discover()){
        w8(stream,0);w8(stream,1);if(wait_bits(stream,1,1))return -1;w8(stream,0);if(wait_bits(stream,1,0))return -1;
        for(int i=0;i<8;i++){bdl[i][0]=(u32)buffers[i];bdl[i][1]=0;bdl[i][2]=sizeof(buffers[i]);bdl[i][3]=0;}memset(buffers,0,sizeof(buffers));
        w32(stream+8,sizeof(buffers));w16(stream+12,7);w16(stream+18,0x11);w32(stream+24,(u32)bdl);w32(stream+28,0);w8(stream+2,0x10);w8(stream+3,0x1c);fill=0;w8(stream,2);active=1;
        print("AUDIO: HDA analog output ready, 48000 Hz stereo PCM\n");return 0;
    }w8(0x4c,0);w8(0x5c,0);return -1;
}
int hda_init(void){active=0;for(int b=0;b<256;b++)for(int d=0;d<32;d++){u32 a=b<<16|d<<11,id=pci(a,0);if((id&65535)==65535)continue;int funcs=pci(a,12)&0x800000?8:1;for(int f=0;f<funcs;f++){u32 af=a|f<<8;if((pci(af,8)>>16)==0x0403&&!controller(af))return 0;}}return -1;}
void hda_pump(void){if(!active)return;u32 pos=r32(stream+4)%sizeof(buffers);int current=pos/sizeof(buffers[0]);for(int guard=0;fill!=current&&guard<8;guard++){audio_fill(buffers[fill],512);fill=(fill+1)&7;}w8(stream+3,0x1c);}
void hda_stop(void){if(!active)return;w8(stream,0);memset(buffers,0,sizeof(buffers));fill=(r32(stream+4)/sizeof(buffers[0]))&7;w8(stream,2);}
