/* Bounded SMF type 0/1 and DOOM MUS scheduling, plus software instrument voices. */
#include "os.h"
extern int mp3_start(const u8*,u32);extern int mp3_sample(short*);
typedef struct{u32 pos,end,next;u8 running,done;}Track;
typedef struct{u32 phase,step;int active,note,channel,velocity,program;}Voice;
static Track tracks[64];static int trackn,kind,looping,owned,volume=80;
static u8*song;static u32 songlen,time,next_mus,division,tempo,muspos,musend,fraction;
static Voice voices[24];static u8 programs[16],volumes[16],musvel[16];static u32 steps[128];
static u32 be32(const u8*p){return p[0]<<24|p[1]<<16|p[2]<<8|p[3];}
static u32 be16(const u8*p){return p[0]<<8|p[1];}
static int variable(u32*pos,u32 end,u32*out){u32 v=0;for(int i=0;i<4;i++){if(*pos>=end)return -1;u8 b=song[(*pos)++];v=v<<7|(b&127);if(!(b&128)){*out=v;return 0;}}return -1;}
static void reset_voices(void){fraction=0;memset(voices,0,sizeof(voices));memset(programs,0,sizeof(programs));for(int i=0;i<16;i++)volumes[i]=musvel[i]=100;steps[69]=38448;for(int n=70;n<128;n++)steps[n]=((unsigned long long)steps[n-1]*69433+32768)>>16;for(int n=68;n>=0;n--)steps[n]=(steps[n+1]*65536u+34716)/69433;}
static void note_off(int ch,int n){for(int i=0;i<24;i++)if(voices[i].active&&voices[i].channel==ch&&voices[i].note==n)voices[i].active=0;}
static void note_on(int ch,int n,int vel){if(n>127||n<0)return;if(!vel){note_off(ch,n);return;}int v=0;for(int i=0;i<24;i++)if(!voices[i].active){v=i;break;}voices[v]=(Voice){0,steps[n],1,n,ch,vel,programs[ch]};}
static void controller(int ch,int c,int value){if(c==7)volumes[ch]=value;if(c==120||c==123)for(int i=0;i<24;i++)if(voices[i].channel==ch)voices[i].active=0;}
static int midi_reset(void){
    reset_voices();time=0;tempo=500000;memset(tracks,0,sizeof(tracks));
    if(songlen<14||memcmp(song,"MThd",4)||be32(song+4)!=6||be16(song+8)>1)return -1;trackn=be16(song+10);division=be16(song+12);if(!division||(division&0x8000)||trackn<1||trackn>64)return -1;
    u32 pos=14;for(int i=0;i<trackn;i++){if(pos+8>songlen||memcmp(song+pos,"MTrk",4))return -1;u32 bytes=be32(song+pos+4);pos+=8;if(bytes>songlen-pos)return -1;Track*t=tracks+i;t->pos=pos;t->end=pos+bytes;if(variable(&t->pos,t->end,&t->next))return -1;pos+=bytes;}return 0;
}
static void midi_event(Track*t){
    if(t->pos>=t->end){t->done=1;return;}u8 st=song[t->pos];if(st&128){t->pos++;if(st<0xf0)t->running=st;}else st=t->running;if(!st){t->done=1;return;}
    if(st==0xff){if(t->pos>=t->end){t->done=1;return;}u8 type=song[t->pos++];u32 size;if(variable(&t->pos,t->end,&size)||size>t->end-t->pos){t->done=1;return;}if(type==0x51&&size==3){u32 tmp=song[t->pos]<<16|song[t->pos+1]<<8|song[t->pos+2];if(tmp)tempo=tmp;}if(type==0x2f)t->done=1;t->pos+=size;}
    else if(st==0xf0||st==0xf7){u32 size;if(variable(&t->pos,t->end,&size)||size>t->end-t->pos){t->done=1;return;}t->pos+=size;t->running=0;}
    else if(st>=0xf0){t->done=1;return;}
    else{int type=st>>4,ch=st&15,n=type==12||type==13?1:2;if((u32)n>t->end-t->pos){t->done=1;return;}u8 a=song[t->pos++],b=n==2?song[t->pos++]:0;if((a|b)&128){t->done=1;return;}if(type==8)note_off(ch,a);else if(type==9)note_on(ch,a,b);else if(type==11)controller(ch,a,b);else if(type==12)programs[ch]=a;}
    if(!t->done){u32 delta;if(variable(&t->pos,t->end,&delta)||delta>0x0fffffff-t->next)t->done=1;else t->next+=delta;}
}
static int mus_reset(void){
    reset_voices();time=next_mus=0;if(songlen<16||memcmp(song,"MUS\x1a",4))return -1;muspos=*(u16*)(song+6);u32 n=*(u16*)(song+4);if(muspos<16||muspos>songlen||n>songlen-muspos)return -1;musend=muspos+n;return 0;
}
static void mus_event(void){
    for(int limit=0;limit<256;limit++){if(muspos>=musend){kind=0;return;}u8 e=song[muspos++];int ch=e&15,type=e>>4&7;if(ch==15)ch=9;else if(ch>=9)ch++;if(type==6){if(looping){mus_reset();return;}kind=0;return;}if(muspos>=musend){kind=0;return;}u8 a=song[muspos++];if(type==0)note_off(ch,a&127);else if(type==1){if(a&128){if(muspos>=musend){kind=0;return;}musvel[ch]=song[muspos++];}note_on(ch,a&127,musvel[ch]&127);}else if(type==2){}else if(type==3){if(a==10||a==11)controller(ch,123,0);}else if(type==4){if(muspos>=musend){kind=0;return;}u8 b=song[muspos++];if(a==0)programs[ch]=b&127;else if(a==3)controller(ch,7,b&127);}else{kind=0;return;}if(e&128){u32 delta;if(variable(&muspos,musend,&delta)){kind=0;return;}next_mus+=delta;return;}}kind=0;
}
void music_stop(void){kind=0;if(owned&&song)dealloc(song);song=0;owned=0;reset_voices();}
void music_volume(int v){volume=v<0?0:v>127?127:v;}
int music_memory(const void*bytes,u32 size,int loop){
    music_stop();if(!audio_available()||!bytes||size<4||size>16*1024*1024)return -1;song=(u8*)bytes;songlen=size;looping=loop;
    if(!memcmp(song,"MThd",4)){kind=1;if(midi_reset())goto bad;print("MIDI software synthesis started\n");}
    else if(!memcmp(song,"MUS\x1a",4)){kind=2;if(mus_reset())goto bad;print("DOOM MUS synthesis started\n");}
    else{kind=3;if(mp3_start(song,songlen))goto bad;print("MP3 decoding started\n");}return 0;
bad:kind=0;song=0;return -1;
}
int music_play(const char*path,int loop){int f=fs_find(path),n=fs_size(f);if(n<4||n>16*1024*1024||!audio_available())return -1;u8*data=alloc(n);if(!data)return -1;if(fs_read(f,0,data,n)!=n){dealloc(data);return -1;}if(music_memory(data,n,loop)){dealloc(data);return -1;}owned=1;return 0;}
/* Tick accumulation uses a rational per-sample remainder, avoiding 64-bit division. */
void music_mix(short*out,u32 frames){
    for(u32 i=0;i<frames&&kind;i++){
        if(kind==3){short s[2];if(mp3_sample(s)){if(looping&&!mp3_start(song,songlen)&&!mp3_sample(s)){}else{kind=0;break;}}out[i*2]=s[0]*volume/127;out[i*2+1]=s[1]*volume/127;continue;}
        if(kind==1){int alive=0;for(int t=0;t<trackn;t++){Track*tr=tracks+t;int limit=512;while(!tr->done&&tr->next<=time&&limit--)midi_event(tr);if(!limit)tr->done=1;if(!tr->done)alive=1;}if(!alive){if(looping){midi_reset();fraction=0;}else{kind=0;break;}}}
        else if(kind==2&&next_mus<=time)mus_event();
        int sum=0;for(int v=0;v<24;v++){Voice*voice=voices+v;if(!voice->active)continue;voice->phase=(voice->phase+voice->step)&0x3fffff;int wave=(voice->phase>>10)-2048;if((voice->program/8)%3==0)wave=voice->phase&0x200000?-1400:1400;else if((voice->program/8)%3==1)wave=wave<0?-wave*2-2048:wave*2-2048;sum+=wave*voice->velocity*volumes[voice->channel]/16384;}
        sum=sum*volume/127;if(sum>32767)sum=32767;if(sum<-32768)sum=-32768;out[i*2]=out[i*2+1]=sum;
        if(kind==1){/* ticks/sample = division/(48 * tempo_in_milliseconds). */u32 den=48*(tempo/1000);if(!den)den=48;fraction+=division;if(fraction>=den){time+=fraction/den;fraction%=den;}}
        else{fraction+=140;if(fraction>=48000){time+=fraction/48000;fraction%=48000;}}
    }
}
