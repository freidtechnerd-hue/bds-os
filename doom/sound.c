/* Original native BDS sound module. DOOM engine remains GPL-2.0-or-later. */
#include "libc.h"
#include "bds.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_config.h"
#include "w_wad.h"
#include "z_zone.h"
extern BdsApi*bds;
int snd_samplerate=48000,snd_cachesize=16*1024*1024,snd_maxslicetime_ms=28;
int snd_musicdevice=SNDDEVICE_SB,snd_sfxdevice=SNDDEVICE_SB;char*snd_musiccmd="";
typedef struct{const unsigned char*data;u32 length;unsigned long long position;u32 step;int volume,sep;}Channel;
typedef struct{void*data;int size;}Song;
static Channel channels[16];static u32 last;static int enabled,music_enabled,music_volume_value=80,playing;
static Song*current_song;static short mixed[4096*2];
void I_InitMusic(void){music_enabled=bds->version>=2&&bds->audio_available()&&!M_CheckParm("-nosound")&&!M_CheckParm("-nomusic");}
void I_InitSound(boolean prefix){enabled=bds->version>=2&&bds->audio_available()&&!M_CheckParm("-nosound")&&!M_CheckParm("-nosfx");last=bds->ticks();I_InitMusic();if(enabled)bds->print("DOOM native PCM sound effects ready\n");}
void I_ShutdownSound(void){enabled=0;if(bds->version>=2)bds->audio_stop();memset(channels,0,sizeof(channels));}
void I_ShutdownMusic(void){music_enabled=playing=0;}
int I_GetSfxLumpNum(sfxinfo_t*sfx){char name[12];snprintf(name,sizeof(name),"ds%s",sfx->name);return W_GetNumForName(name);}
void I_UpdateSoundParams(int ch,int volume,int sep){if(ch>=0&&ch<16){channels[ch].volume=volume;channels[ch].sep=sep;}}
int I_StartSound(sfxinfo_t*sfx,int ch,int volume,int sep){
    if(!enabled||ch<0||ch>=16)return -1;int lump=I_GetSfxLumpNum(sfx),size=W_LumpLength(lump);if(size<40)return -1;
    const unsigned char*d=W_CacheLumpNum(lump,PU_STATIC);u32 samples=d[4]|d[5]<<8|d[6]<<16|d[7]<<24,rate=d[2]|d[3]<<8;
    if(d[0]!=3||d[1]||samples<32||samples>(u32)size-8||rate<4000||rate>48000)return -1;
    channels[ch]=(Channel){d+24,samples-32,0,rate*65536/48000,volume,sep};return ch;
}
void I_StopSound(int ch){if(ch>=0&&ch<16)channels[ch].data=0;}
boolean I_SoundIsPlaying(int ch){return ch>=0&&ch<16&&channels[ch].data!=NULL;}
void I_PrecacheSounds(sfxinfo_t*sounds,int n){}
void I_UpdateSound(void){
    if(!enabled)return;u32 now=bds->ticks(),elapsed=now-last;last=now;if(elapsed>80)elapsed=80;u32 frames=elapsed*48;if(!frames)return;
    for(u32 i=0;i<frames;i++){int left=0,right=0;for(int c=0;c<16;c++){Channel*ch=channels+c;if(!ch->data)continue;u32 p=ch->position>>16;if(p>=ch->length){ch->data=NULL;continue;}int v=((int)ch->data[p]-128)*ch->volume;left+=v*(254-ch->sep)/254;right+=v*ch->sep/254;ch->position+=ch->step;}if(left>32767)left=32767;if(left<-32768)left=-32768;if(right>32767)right=32767;if(right<-32768)right=-32768;mixed[i*2]=left;mixed[i*2+1]=right;}
    bds->audio_write(mixed,frames,48000,2);
}
void I_SetMusicVolume(int volume){music_volume_value=volume;if(bds->version>=2)bds->music_volume(volume);}
void I_PauseSong(void){if(bds->version>=2)bds->music_volume(0);}
void I_ResumeSong(void){if(bds->version>=2)bds->music_volume(music_volume_value);}
void*I_RegisterSong(void*data,int len){if(!music_enabled||len<4)return NULL;Song*s=malloc(sizeof(*s));if(s){s->data=data;s->size=len;}return s;}
void I_UnRegisterSong(void*handle){if(handle==current_song){playing=0;current_song=NULL;}free(handle);}
void I_PlaySong(void*handle,boolean loop){Song*s=handle;if(s&&music_enabled){current_song=s;playing=bds->music_data(s->data,s->size,loop)==0;}}
void I_StopSong(void){playing=0;if(bds->version>=2)bds->music_data(NULL,0,0);}
boolean I_MusicIsPlaying(void){return playing;}
void I_BindSoundVariables(void){M_BindVariable("snd_musicdevice",&snd_musicdevice);M_BindVariable("snd_sfxdevice",&snd_sfxdevice);M_BindVariable("snd_samplerate",&snd_samplerate);M_BindVariable("snd_cachesize",&snd_cachesize);M_BindVariable("snd_maxslicetime_ms",&snd_maxslicetime_ms);M_BindVariable("snd_musiccmd",&snd_musiccmd);}
