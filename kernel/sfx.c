#include "os.h"
/* Original short triangle-wave chimes, generated without a sample bank. */
static int enabled=1;
int system_sounds(int value){if(value>=0)enabled=!!value;return enabled;}
void system_sound(int kind){
    if(!enabled||!audio_available()||audio_is_muted())return;
    short pcm[8192];int count=kind==0?8192:2400;
    for(int i=0;i<count;i++){
        int segment=kind==0?i/2731:0,freq=kind==0?(segment==0?523:segment==1?659:784):kind==2?196:880;
        int phase=(i*freq%16000)*4/16;int wave=phase<2000?phase-1000:3000-phase;
        int envelope=(count-i)*3;if(i<160)envelope=envelope*i/160;
        pcm[i]=wave*envelope/count;
    }
    audio_write(pcm,count,16000,1);audio_pump();serial(kind==0?"SFX startup chime queued\n":kind==2?"SFX error\n":"SFX window\n");
}
