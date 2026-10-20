#include "os.h"
/* VESA VBE 2/3 real-mode mode discovery. Only validated RGB32 LFB modes. */
DisplayMode display_modes[32];int display_count;static int current=-1;
static const u16 sizes[][2]={{640,480},{800,600},{1024,768},{1280,720},{1280,800},{1280,1024},{1360,768},{1366,768},{1440,900},{1600,900},{1920,1080},{1920,1200},{1680,1050},{2560,1080},{2560,1440},{2560,1600},{3440,1440},{3840,2160}};
static void log_mode(const char*prefix){char n[16];serial(prefix);number(n,screen_w);serial(n);serial("x");number(n,screen_h);serial(n);serial("\n");}
int display_current(void){return current;}
int display_set(int index){
    if(index<0||index>=display_count||current<0)return -1;if(index==current)return 0;
    DisplayMode*m=display_modes+index;u16 actual=0;
    if(bios_video(0x4f02,m->id|0x4000,0,0)||bios_video(0x4f03,0,0,&actual)||(actual&0x3fff)!=m->id){
        bios_video(0x4f02,display_modes[current].id|0x4000,0,0);serial("DISPLAY: mode rejected\n");return -1;
    }
    video_mode(m);current=index;log_mode("DISPLAY: switched ");return 0;
}
static int config_path(char*p){return fs_qualify(fs_system_volume,"/DISPLAY.CFG",p,64);}
int display_save(void){
    char path[64],config[32],n[16];if(config_path(path))return -1;
    number(config,screen_w);strcpy(config+strlen(config)," ");number(n,screen_h);strcpy(config+strlen(config),n);strcpy(config+strlen(config),"\n");
    if(fs_write(path,config,strlen(config)))return -1;log_mode("DISPLAY: saved ");return 0;
}
void display_init(int load_saved){
    display_count=0;current=-1;if(!graphics)return;
    u8*b=(u8*)0x7400;memset(b,0,512);memcpy(b,"VBE2",4);
    if(bios_video(0x4f00,0,0,0)||memcmp(b,"VESA",4)||*(u16*)(b+4)<0x200)return;
    int version=*(u16*)(b+4);u32 farptr=*(u32*)(b+14),address=(farptr>>16)*16+(farptr&65535);
    if(address<0x500||address>=0x100000)return;
    /* Copy before subsequent BIOS calls overwrite the controller-info buffer. */
    u16 ids[256];int count=0;while(count<256&&address+count*2+2<=0x100000){u16 id=*(volatile u16*)(address+count*2);if(id==0xffff)break;ids[count++]=id;}
    u16 active;if(bios_video(0x4f03,0,0,&active))return;active&=0x3fff;
    for(int i=0;i<count;i++){
        memset(b,0,256);if(bios_video(0x4f01,0,ids[i],0))continue;
        u16 flags=*(u16*)b,w=*(u16*)(b+18),h=*(u16*)(b+20);
        if((flags&0x99)!=0x99||b[25]!=32||b[27]!=6)continue;
        unsigned s=0;while(s<sizeof(sizes)/sizeof(sizes[0])&&(w!=sizes[s][0]||h!=sizes[s][1]))s++;
        if(s==sizeof(sizes)/sizeof(sizes[0]))continue;
        int duplicate=0;for(int j=0;j<display_count;j++)if(display_modes[j].w==w&&display_modes[j].h==h)duplicate=1;if(duplicate||display_count>=32)continue;
        u32 stride=*(u16*)(b+16),fb=*(u32*)(b+40);int masks=31;
        if(version>=0x300&&*(u16*)(b+50)){stride=*(u16*)(b+50);masks=54;}
        if(stride<w*4u||stride>16384||(stride&3)||fb<0x7000000||fb>0xffffffff-stride*h)continue;
        if(b[masks]!=8||b[masks+2]!=8||b[masks+4]!=8)continue;
        u8 r=b[masks+1],g=b[masks+3],blue=b[masks+5];if(r>24||g>24||blue>24||((255u<<r)&(255u<<g))||((255u<<r)&(255u<<blue))||((255u<<g)&(255u<<blue)))continue;
        int n=0;while(n<display_count&&(display_modes[n].w<w||(display_modes[n].w==w&&display_modes[n].h<h)))n++;
        for(int j=display_count;j>n;j--)display_modes[j]=display_modes[j-1];display_count++;
        display_modes[n]=(DisplayMode){ids[i],w,h,fb,stride,r,g,blue};
    }
    /* GRUB may use its native Bochs driver, leaving BIOS mode 3 active.
       A matching framebuffer layout is also a safe VBE restore target. */
    for(int i=0;i<display_count;i++)if(video_matches(display_modes+i))current=i;
    {char n[16];number(n,active);serial("DISPLAY: firmware current mode ");serial(n);number(n,display_count);serial("; usable modes ");serial(n);serial("\n");}
    if(current<0){display_count=0;serial("DISPLAY: current mode cannot be restored; switching disabled\n");return;}
    serial("DISPLAY: supported");for(int i=0;i<display_count;i++){char n[16];serial(" ");number(n,display_modes[i].w);serial(n);serial("x");number(n,display_modes[i].h);serial(n);}serial("\n");
    if(load_saved){char path[64],config[32];if(config_path(path))return;int f=fs_find(path),n=fs_size(f);if(n<4||n>=32||fs_read(f,0,config,n)!=n)return;config[n]=0;
        int p=0,w=0,h=0;while(config[p]>='0'&&config[p]<='9'&&p<4)w=w*10+config[p++]-'0';if(config[p++]!=' ')return;
        int first=p;while(config[p]>='0'&&config[p]<='9'&&p-first<4)h=h*10+config[p++]-'0';if(p==first||config[p]!='\n'||config[p+1])return;
        for(int i=0;i<display_count;i++)if(display_modes[i].w==w&&display_modes[i].h==h){if(!display_set(i))log_mode("DISPLAY: restored ");return;}
        serial("DISPLAY: saved size unavailable; using boot mode\n");
    }
}
