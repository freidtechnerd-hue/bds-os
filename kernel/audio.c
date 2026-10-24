/* Intel ICH AC'97 PCM-out bus mastering; polling avoids firmware IRQ conflicts. */
#include "os.h"
#define FRAMES 512
#define RING 32768
static u16 mixer,busmaster;static int ready,muted,busy,tail,backend;
extern int hda_init(void);extern void hda_pump(void),hda_stop(void);
static u32 descriptors[32][2] __attribute__((aligned(16)));
static short dma[32][FRAMES*2] __attribute__((aligned(16)));
static short ring[RING*2];static u32 rp,wp;
static int paused,master=100;
int audio_volume(int value){if(value>=0)master=value>100?100:value;return master;}
void audio_pause(int value){paused=!!value;}
u32 audio_pending(void){audio_pump();return (wp-rp)*1000/48000;}
static u32 pci(u32 address,int reg){outl(0xcf8,0x80000000|address|reg);return inl(0xcfc);}
int audio_available(void){return ready;}
int audio_is_muted(void){return muted;}
void audio_mute(int state){muted=!!state;if(ready&&backend==1)outw(mixer+0x18,muted?0x8000:0x0808);print(muted?"AUDIO muted\n":"AUDIO unmuted\n");}
void audio_init(void){
    ready=backend=0;u32 found=0xffffffff;
    for(int b=0;b<256&&found==0xffffffff;b++)for(int d=0;d<32&&found==0xffffffff;d++){u32 a=b<<16|d<<11,id=pci(a,0);if((id&65535)==65535)continue;int funcs=pci(a,0x0c)&0x800000?8:1;for(int f=0;f<funcs;f++){u32 af=a|f<<8,v=pci(af,0),dev=v>>16;if((v&65535)==0x8086&&(dev==0x2415||dev==0x2425||dev==0x2445||dev==0x2485||dev==0x24c5||dev==0x24d5||dev==0x266e||dev==0x27de))found=af;}}
    if(found==0xffffffff){if(!hda_init()){backend=2;ready=1;rp=wp=0;return;}print("AUDIO: no supported AC'97/HDA analog controller; silent mode\n");return;}
    u32 m=pci(found,0x10),bm=pci(found,0x14);if(!(m&1)||!(bm&1)||(m&~3u)>0xff00||(bm&~3u)>0xff00){print("AUDIO: unsupported AC'97 BARs\n");return;}
    mixer=m&~3u;busmaster=bm&~3u;u16 command=pci(found,4);outl(0xcf8,0x80000000|found|4);outw(0xcfc,command|5);
    outl(busmaster+0x2c,2);u32 end=ticks()+500;while(!(inl(busmaster+0x30)&0x100)&&(int)(end-ticks())>0)sleep(10);
    if(!(inl(busmaster+0x30)&0x100)){print("AUDIO: AC'97 codec did not become ready\n");return;}
    outw(mixer+0x26,0);outw(mixer+2,0);outw(mixer+0x18,0x0808);outw(mixer+0x2a,0); /* fixed 48 kHz */
    outb(busmaster+0x1b,2);for(int i=0;i<10000&&(inb(busmaster+0x1b)&2);i++);
    memset(dma,0,sizeof(dma));for(int i=0;i<32;i++){descriptors[i][0]=(u32)dma[i];descriptors[i][1]=FRAMES*2;}
    outl(busmaster+0x10,(u32)descriptors);tail=0;outb(busmaster+0x15,0);outb(busmaster+0x1b,1);ready=1;backend=1;rp=wp=0;print("AUDIO: Intel AC'97 ready, 48000 Hz stereo PCM\n");
}
static short clamp(int v){return v>32767?32767:v<-32768?-32768:v;}
void audio_fill(short*p,u32 frames){memset(p,0,frames*4);if(paused)return;music_mix(p,frames);for(u32 i=0;i<frames;i++)if(rp!=wp){int r=rp%RING;p[i*2]=clamp(p[i*2]+ring[r*2]);p[i*2+1]=clamp(p[i*2+1]+ring[r*2+1]);rp++;}sfx_mix(p,frames);if(muted)memset(p,0,frames*4);else if(master!=100)for(u32 i=0;i<frames*2;i++)p[i]=p[i]*master/100;}
void audio_pump(void){
    if(!ready||busy)return;busy=1;if(backend==2){hda_pump();busy=0;return;}int civ=inb(busmaster+0x14)&31;u16 status=inw(busmaster+0x16);int queued=(tail-civ+32)%32+1;if((status&3)==3)queued=0;
    while(queued<4){tail=(tail+1)&31;audio_fill(dma[tail],FRAMES);outb(busmaster+0x15,tail);queued++;}
    outw(busmaster+0x16,status&0x1c);if(status&1)outb(busmaster+0x1b,1);busy=0;
}
int audio_write(const short*pcm,u32 frames,u32 rate,int channels){
    if(!ready)return -1;if(!pcm||!frames||frames>8192||rate<8000||rate>96000||(channels!=1&&channels!=2))return -1;audio_pump();u32 free=RING-(wp-rp);u32 accepted=frames;if(accepted>free*rate/48000)accepted=free*rate/48000;u32 count=accepted*48000/rate;
    for(u32 i=0;i<count;i++){u32 source=i*rate/48000,r=wp++%RING;ring[r*2]=pcm[source*channels];ring[r*2+1]=pcm[source*channels+(channels==2)];}return accepted;
}
void audio_stop(void){paused=0;rp=wp=0;if(ready&&backend==2){hda_stop();return;}if(ready){outb(busmaster+0x1b,0);memset(dma,0,sizeof(dma));outb(busmaster+0x1b,2);for(int i=0;i<10000&&(inb(busmaster+0x1b)&2);i++);outl(busmaster+0x10,(u32)descriptors);tail=0;outb(busmaster+0x15,0);outb(busmaster+0x1b,1);}}
