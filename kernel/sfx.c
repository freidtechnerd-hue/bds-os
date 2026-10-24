#include "os.h"
/* Original short triangle-wave chimes, generated without a sample bank. */
static int enabled=1;static short samples[4][65536];static u32 lengths[4],positions[4];
void sfx_init(void){
    const char*names[]={"START","CLICK","ERROR","DING"};
    for(int i=0;i<4;i++){char path[256],name[64]="/SOUNDS/";strcpy(name+8,names[i]);strcpy(name+strlen(name),".WAV");if(fs_qualify(fs_system_volume,name,path,sizeof(path)))continue;
        int f=fs_find(path),n=fs_size(f);u8 h[16];if(n<44||n>256*1024||fs_read(f,0,h,12)!=12||memcmp(h,"RIFF",4)||memcmp(h+8,"WAVE",4))continue;u32 pos=12;int valid=0;
        while(pos+8<=(u32)n){if(fs_read(f,pos,h,8)!=8)break;u32 size=*(u32*)(h+4);if(size>(u32)n-pos-8)break;
            if(!memcmp(h,"fmt ",4)&&size>=16){if(fs_read(f,pos+8,h,16)!=16)break;valid=*(u16*)h==1&&*(u16*)(h+2)==1&&*(u32*)(h+4)==48000&&*(u16*)(h+14)==16;}
            else if(!memcmp(h,"data",4)&&valid&&size<=sizeof(samples[i])&&!(size&1)){if(fs_read(f,pos+8,samples[i],size)==(int)size)lengths[i]=size/2;break;}
            pos+=8+size+(size&1);
        }positions[i]=lengths[i];
    }
    serial("SFX user sound bank loaded\n");
}
void sfx_mix(short*out,u32 frames){
    if(!enabled)return;for(u32 i=0;i<frames;i++){int sample=0;for(int j=0;j<4;j++)if(positions[j]<lengths[j])sample+=samples[j][positions[j]++];
        for(int c=0;c<2;c++){int v=out[i*2+c]+sample;out[i*2+c]=v>32767?32767:v<-32768?-32768:v;}}
}
int system_sounds(int value){if(value>=0)enabled=!!value;return enabled;}
void system_sound(int kind){
    if(!enabled||!audio_available()||audio_is_muted())return;
    if(kind>=0&&kind<4&&lengths[kind]){positions[kind]=0;audio_pump();serial(kind==0?"SFX startup chime queued\n":kind==2?"SFX error\n":kind==3?"SFX notification\n":"SFX window\n");return;}
    short pcm[8192];int count=kind==0?8192:2400;
    for(int i=0;i<count;i++){
        int segment=kind==0?i/2731:0,freq=kind==0?(segment==0?523:segment==1?659:784):kind==2?196:880;
        int phase=(i*freq%16000)*4/16;int wave=phase<2000?phase-1000:3000-phase;
        int envelope=(count-i)*3;if(i<160)envelope=envelope*i/160;
        pcm[i]=wave*envelope/count;
    }
    audio_write(pcm,count,16000,1);audio_pump();serial(kind==0?"SFX startup chime queued\n":kind==2?"SFX error\n":"SFX window\n");
}
