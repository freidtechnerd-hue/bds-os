#include "common.h"
static void value(BdsApi*a,int y,const char*label,u32 n){char b[24];itoa(b,n);a->text(32,y,label,0xc7d5e0);a->text(360,y,b,0xffffff);}
int app_main(BdsApi*a,int argc,char**argv){
    if(a->version<7){a->print("System monitor requires API 7\n");return 1;}
    a->print("SYSTEM MONITOR ready\n");
    for(;;){BdsSystemInfo s;if(a->system_info(&s,sizeof(s)))return 1;
        a->rect(0,0,800,600,0x172531);a->text(32,32,"BDX-OS - System Monitor",0xffffff);
        value(a,88,"Native architecture (bits)",s.arch_bits);value(a,120,"Application API version",s.abi);
        value(a,152,"Uptime (seconds)",s.uptime_ms/1000);value(a,184,"App heap used (KiB)",s.heap_used/1024);
        value(a,216,"App heap capacity (MiB)",s.heap_total/1048576);value(a,248,"Screen width",s.screen_width);value(a,280,"Screen height",s.screen_height);
        value(a,312,"Presented app frames",s.frames);value(a,344,"Last frame interval (ms)",s.last_frame_ms);
        a->text(32,408,"Cooperative native apps share the kernel address space.",0xd8c582);
        a->text(32,440,"ESC or F10 closes | Ctrl+F toggles fullscreen",0xc7d5e0);a->present();
        int k=a->key(0);if(k==27||k==KEY_F10)return 0;a->sleep(250);
    }
}
