#include "os.h"
/* Outside the per-app heap: survives app exit, never persisted to disk. */
static char clipboard[4096];static u32 clipboard_size;
int clipboard_set(const char*s,u32 n){if(!s||n>=sizeof(clipboard))return -1;memcpy(clipboard,s,n);clipboard[n]=0;clipboard_size=n;return 0;}
int clipboard_get(char*d,u32 cap){if(!d||cap<=clipboard_size)return -1;memcpy(d,clipboard,clipboard_size+1);return clipboard_size;}
static u32 frames,last_frame,previous;
void frame_record(void){u32 now=ticks();last_frame=previous?now-previous:0;previous=now;frames++;}
int system_info(BdsSystemInfo*out,u32 size){
    if(!out||size<sizeof(*out))return -1;
    *out=(BdsSystemInfo){sizeof(*out),7,sizeof(void*)*8,ticks(),heap_used(),0x4000000,screen_w,screen_h,frames,last_frame};return 0;
}
