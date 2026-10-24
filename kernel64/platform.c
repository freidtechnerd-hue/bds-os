/* Explicitly unavailable drivers in the x64 preview; no fake success. */
#include "os.h"
DisplayMode display_modes[32];int display_count;
void display_init(int automatic){display_modes[0]=(DisplayMode){0,screen_w,screen_h,0,0,16,8,0};display_count=graphics?1:0;}
int display_current(void){return display_count?0:-1;}int display_set(int i){return i==0&&display_count?0:-1;}
int display_save(void){return -1;}int display_auto_save(void){return -1;}int display_auto_index(void){return display_current();}int display_automatic(void){return 0;}
void audio_init(void){}int audio_available(void){return 0;}int audio_write(const short*p,u32 n,u32 rate,int channels){return -1;}
void audio_stop(void){}void audio_pump(void){}void audio_mute(int n){}int audio_is_muted(void){return 1;}void audio_pause(int p){}u32 audio_pending(void){return 0;}
int audio_volume(int n){return 0;}void system_sound(int n){}int system_sounds(int n){return 0;}
void music_stop(void){}int music_play(const char*p,int loop){return -1;}int music_memory(const void*p,u32 n,int loop){return -1;}void music_volume(int n){}int music_control(int a,int b){return -1;}
void net_init(void){}void net_poll(void){}int http_get(const char*p,char*out,u32 n){return -1;}
int net_info(char*out,u32 n){const char*s="Networking is not implemented in the x64 preview.";if(n<=strlen(s))return -1;strcpy(out,s);return 0;}
int cd_count(void){return 0;}int cd_info(int n,CdInfo*out){return -1;}int cd_read(int d,u32 lba,u32 n,void*out){return -1;}
