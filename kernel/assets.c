#include "os.h"
typedef struct{char path[256];u32 pixels[256];}Icon;
static Icon cache[64];static int used;
/* User-supplied arrow reference: X black outline, O white fill, . transparent. */
void cursor_draw(void){
    static const char*shape[]={
        "X...........","XX..........","XOX.........","XOOX........",
        "XOOOX.......","XOOOOX......","XOOOOOX.....","XOOOOOOX....",
        "XOOOOOOOX...","XOOOOOOOOX..","XOOOOOOOOOX.","XOOOOOOOOOOX",
        "XOOOOOOXXXXX","XOOOXOOX....","XOOX.XOOX...","XOX..XOOX...",
        "XX....XOOX..","......XOOX..",".......XX..."};
    for(int y=0;y<19;y++)for(int x=0;x<12;x++)if(shape[y][x]!='.')rect(mouse_x+x,mouse_y+y,1,1,shape[y][x]=='X'?0:0xffffff);
}
void icons_reset(void){used=0;}
static int ico_load(int f,u32*out){
    u8 b[2048];int n=fs_size(f);u32 offset=0;RunHeader h;
    if(n>=32&&fs_read(f,0,&h,32)==32&&!memcmp(h.magic,"BDSR",4)){
        if(h.version!=2||h.arch!=2||h.flags<22||h.flags>2048||h.code>(u32)n-32||h.flags!=(u32)n-32-h.code)return -1;offset=32+h.code;n=h.flags;
    }
    if(n<22||n>(int)sizeof(b)||fs_read(f,offset,b,n)!=n)return -1;
    if(*(u16*)b||*(u16*)(b+2)!=1||!*(u16*)(b+4)||b[6]!=16||b[7]!=16)return -1;
    u32 off=*(u32*)(b+18);if(off>(u32)n||1128>(u32)n-off)return -1;u8*d=b+off;
    if(*(u32*)d!=40||*(u32*)(d+4)!=16||*(u32*)(d+8)!=32||*(u16*)(d+12)!=1||*(u16*)(d+14)!=32||*(u32*)(d+16))return -1;
    for(int y=0;y<16;y++)for(int x=0;x<16;x++){u8*p=d+40+(15-y)*64+x*4;int mask=d[40+1024+(15-y)*4+x/8]&(128>>(x&7));out[y*16+x]=mask?0:0xff000000|p[2]<<16|p[1]<<8|p[0];}return 0;
}
void icon_draw(const char*path,int x,int y,int scale){
    char name[256],qualified[256];int n=strlen(path),isrun=n>=4&&!stricmp(path+n-4,".RUN"),isico=n>=4&&!stricmp(path+n-4,".ICO");const char*kind="RUN";
    if(n>=4&&!stricmp(path+n-4,".BIN"))kind="BIN";else if(n>=4&&!stricmp(path+n-4,".DLL"))kind="DLL";else if(n>=4&&!stricmp(path+n-4,".DCL"))kind="DCL";else if(n>=4&&!stricmp(path+n-4,".NES"))kind="NES";else if(n>=4&&(!stricmp(path+n-4,".MP3")||!stricmp(path+n-4,".MID")))kind="PLAYER";else if(n>=4&&(!stricmp(path+n-4,".BMP")||!stricmp(path+n-4,".PNG")))kind="IMAGE";else if(n>=4&&!stricmp(path+n-4,".WAD"))kind="DOOM";else if((n>=4&&!stricmp(path+n-4,".HTM"))||(n>=5&&!stricmp(path+n-5,".HTML")))kind="BROWSER";else if(n>=4&&!stricmp(path+n-4,".PSF"))kind="FONT";else if(n>=5&&!stricmp(path+n-5,".MIDI"))kind="PLAYER";else{int simple=1;for(int i=0;i<n;i++)if(path[i]=='.'||path[i]=='/'||path[i]==':'||path[i]=='\\')simple=0;if(simple&&n<32)kind=path;}
    if(isrun||isico){if(fs_qualify(fs_volume(),path,qualified,sizeof(qualified)))return;}
    else{strcpy(name,"/ICONS/");strcpy(name+7,kind);strcpy(name+strlen(name),".ICO");if(fs_qualify(fs_system_volume,name,qualified,sizeof(qualified)))return;}
    Icon*icon=0;for(int i=0;i<used;i++)if(!stricmp(cache[i].path,qualified)){icon=cache+i;break;}
    if(!icon){if(used==64)used=0;icon=cache+used;strcpy(icon->path,qualified);int ok=-1;if(!isrun&&!isico){char app[64],q[256];strcpy(app,"/");strcpy(app+1,kind);strcpy(app+strlen(app),".RUN");if(!fs_qualify(fs_system_volume,app,q,sizeof(q)))ok=ico_load(fs_find(q),icon->pixels);}if(ok)ok=ico_load(fs_find(qualified),icon->pixels);
        if(ok&&isrun){strcpy(name,qualified);strcpy(name+strlen(name)-4,".ICO");ok=ico_load(fs_find(name),icon->pixels);}
        if(ok){strcpy(name,"/ICONS/");strcpy(name+7,kind);strcpy(name+strlen(name),".ICO");if(!fs_qualify(fs_system_volume,name,qualified,sizeof(qualified)))ok=ico_load(fs_find(qualified),icon->pixels);}
        if(ok)icon=0;else used++;
    }
    if(icon){for(int j=0;j<16;j++)for(int i=0;i<16;i++)if(icon->pixels[j*16+i]>>24)rect(x+i*scale,y+j*scale,scale,scale,icon->pixels[j*16+i]);}
    else{rect(x,y,16*scale,16*scale,0xcbd7c4);rect(x+3*scale,y+4*scale,10*scale,8*scale,0x3e6950);}
}
int bmp_draw(const char*path){
    int f=fs_find(path),n=fs_size(f);if(n<54||n>4*1024*1024)return -1;u8*b=alloc(n);if(!b)return -1;
    int r=-1;if(fs_read(f,0,b,n)!=n)goto done;
    u32 off=*(u32*)(b+10),dib=*(u32*)(b+14);int w=*(int*)(b+18),h=*(int*)(b+22);u16 bits=*(u16*)(b+28);
    if(b[0]!='B'||b[1]!='M'||dib<40||dib>(u32)n-14||off<14+dib||*(u16*)(b+26)!=1||*(u32*)(b+30)||(bits!=24&&bits!=32)||w<1||w>1024||!h||h<-768||h>768)goto done;
    int height=h<0?-h:h;u32 row=(w*(bits/8)+3)&~3u;if(off>(u32)n||row*height>(u32)n-off)goto done;
    int dw=screen_w,dh=dw*height/w;if(dh>screen_h){dh=screen_h;dw=dh*w/height;}int ox=(screen_w-dw)/2,oy=(screen_h-dh)/2;
    rect(0,0,screen_w,screen_h,0x17221c);
    for(int y=0;y<dh;y++){u8*p=b+off+(h>0?height-1-y*height/dh:y*height/dh)*row;for(int x=0;x<dw;x++){u8*c=p+(x*w/dw)*(bits/8);rect(ox+x,oy+y,1,1,c[2]<<16|c[1]<<8|c[0]);}}r=0;
done:dealloc(b);return r;
}
/* Compact persistent cache survives the native-app heap reset. */
static u32 wallpaper[200*150];static int wallpaper_ready;
void wallpaper_init(void){
    int f=fs_find("/WALLPAPR.BMP");u8 header[54],row[600];wallpaper_ready=0;
    if(fs_read(f,0,header,54)!=54||memcmp(header,"BM",2)||*(u32*)(header+14)!=40||*(u32*)(header+18)!=200||*(u32*)(header+22)!=150||*(u16*)(header+26)!=1||*(u16*)(header+28)!=24||*(u32*)(header+30))return;
    u32 off=*(u32*)(header+10);int n=fs_size(f);if(off<54||off>(u32)n||90000>(u32)n-off)return;
    for(int y=0;y<150;y++){if(fs_read(f,off+(149-y)*600,row,600)!=600)return;for(int x=0;x<200;x++)wallpaper[y*200+x]=row[x*3+2]<<16|row[x*3+1]<<8|row[x*3];}
    wallpaper_ready=1;serial("WALLPAPER cloud image cached\n");
}
void wallpaper_draw(void){if(wallpaper_ready)video_stretch(wallpaper,200,150);else rect(0,0,screen_w,screen_h,0x202b27);}
void boot_splash(void){
    if(!graphics)return;if(bmp_draw("/BOOT.BMP")){rect(0,0,screen_w,screen_h,0x17221c);text(screen_w/2-72,screen_h/2,"BDS-OS ULTIMATE",0xe8d49a);}
    int y=screen_h-70;rect(40,y,screen_w-80,38,0x17221c);text(52,y+8,"Starting desktop, native apps and audio...",0xdde9d6);
    u32 start=ticks();serial("BOOT BMP splash ready\n");system_sound(0);
    do{u32 elapsed=ticks()-start;if(elapsed>3000)elapsed=3000;rect(52,y+24,screen_w-104,5,0x314439);rect(52,y+24,(screen_w-104)*elapsed/3000,5,0xa3dfaa);present();if(elapsed==3000)break;sleep(30);}while(1);
    serial("BOOT progress complete after 3000 ms\n");
}
