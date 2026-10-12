#include "os.h"
#include "storage.h"
static u32 surface[800*600];static BdsApi*current;
static int full,menu,prev,shown,xpos,ypos,cw,ch,opening,closing;static char title[256],openpath[256];
static const char*appname(void){const char*p=title;for(const char*s=title;*s;s++)if(*s=='/')p=s;return p;}
int app_pointer(int*x,int*y){input_poll();int ox=full?0:xpos,oy=full?0:ypos+48,w=full?screen_w:cw,h=full?screen_h:ch;*x=*y=-1;if(!current||menu||mouse_x<ox||mouse_x>=ox+w||mouse_y<oy||mouse_y>=oy+h)return 0;*x=(mouse_x-ox)*800/w;*y=(mouse_y-oy)*600/h;return mouse_buttons;}
extern u8 font_row(u8,int);
void app_rect(int x,int y,int w,int h,u32 c){if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}if(x+w>800)w=800-x;if(y+h>600)h=600-y;for(int j=0;j<h;j++)for(int i=0;i<w;i++)surface[(y+j)*800+x+i]=c;}
void app_text(int x,int y,const char*s,u32 c){int base=x;while(*s){u8 k=*s++;if(k=='\n'){x=base;y+=12;continue;}for(int j=0;j<8;j++)for(int i=0;i<8;i++)if(font_row(k,j)&(1<<i))app_rect(x+i,y+j,1,1,c);x+=8;}}
void app_fullscreen(int value){if(!current)return;full=value<0?!full:!!value;menu=0;serial(full?"APP fullscreen enabled\n":"APP windowed enabled\n");}
void app_begin(BdsApi*api,const char*path){current=api;full=menu=shown=opening=closing=0;prev=mouse_buttons&1;strcpy(title,path);memset(surface,0,sizeof(surface));cw=screen_w-160;if(cw>640)cw=640;if(cw<320)cw=320;ch=cw*3/4;if(ch>screen_h-112){ch=screen_h-112;cw=ch*4/3;}xpos=(screen_w-cw)/2;ypos=(screen_h-ch-52)/2;serial("APP windowed launch: ");serial(path);serial("\n");}
void app_end(void){current=0;menu=0;}
static void action(int row){
    if(menu==1){if(row==0){char p[256];const char*filter=!stricmp(appname(),"/NES.RUN")?".NES":!stricmp(appname(),"/PLAYER.RUN")?".MP3;.MID;.MIDI":!stricmp(appname(),"/IMAGE.RUN")?".BMP;.PNG":!stricmp(appname(),"/DOOM.RUN")?".WAD":!stricmp(appname(),"/BROWSER.RUN")?".HTM;.HTML;.TXT":0;if(!file_dialog(filter,p,sizeof(p))){strcpy(openpath,p);opening=1;print("File selected: ");print(p);print("\n");}}else if(row==1)closing=1;}
    else if(menu==2){if(row==0)app_fullscreen(-1);else if(row==1){char p[256];if(!file_dialog(".PSF",p,sizeof(p))&&font_load(p))print("FONT: invalid PSF1 font\n");}else if(row==2)font_reset();else if(row==3)audio_mute(!audio_is_muted());else if(row==4){char p[256];if(!file_dialog(".MP3;.MID;.MIDI",p,sizeof(p))&&music_play(p,1))print("AUDIO: cannot play file\n");}else if(row==5)music_stop();}
    else if(menu==3)print("BDS app: Ctrl+F fullscreen | File and Options menus | F10/ESC app controls\n");menu=0;
}
void app_poll(void){
    if(!current||full)return;int down=mouse_buttons&1,clicked=down&&!prev;prev=down;if(!clicked)return;
    int x=mouse_x-xpos,y=mouse_y-ypos;
    if(y>=0&&y<26&&x>=cw-24&&x<cw){closing=1;return;}
    if(y>=28&&y<48){if(x>=8&&x<58)menu=menu==1?0:1;else if(x>=62&&x<134)menu=menu==2?0:2;else if(x>=138&&x<190)menu=menu==3?0:3;return;}
    if(menu&&x>=8&&x<260&&y>=50){int row=(y-50)/24;if(row<6)action(row);else menu=0;}else menu=0;
}
void app_present(void){
    if(!current)return;audio_pump();net_poll();app_poll();if(!shown){shown=1;serial("APP window frame ready\n");}
    if(!full){desktop_background();rect(xpos-4,ypos-4,cw+8,ch+56,0xc5c8c0);rect(xpos,ypos,cw,26,0x36594b);icon_draw(title,xpos+6,ypos+5,1);text(xpos+28,ypos+9,title,0xffffff);text(xpos+cw-18,ypos+9,"X",0xffffff);text(xpos+8,ypos+34,"File   Options   Help",0x17211d);}
    int ox=full?0:xpos,oy=full?0:ypos+48,w=full?screen_w:cw,h=full?screen_h:ch;
    video_scaled(surface,800,600,ox,oy,w,h);
    if(!full&&menu){const char*file[]={"Open file...","Exit application"};const char*options[]={"Fullscreen  Ctrl+F","Load custom PSF font...","Use built-in font","Mute / unmute audio","Play MP3 / MIDI...","Stop music"};const char*help[]={"Show controls in Terminal"};const char**items=menu==1?file:menu==2?options:help;int n=menu==1?2:menu==2?6:1;rect(xpos+8,ypos+50,252,n*24,0xc5c8c0);for(int i=0;i<n;i++)text(xpos+16,ypos+58+i*24,items[i],0x17211d);}
    cursor_draw();present();
}
void app_blit(const u32*src,int w,int h){if(w<1||h<1||w>1024||h>768)return;int scale=800/w;if(600/h<scale)scale=600/h;if(scale<1)scale=1;int ox=(800-w*scale)/2,oy=(600-h*scale)/2;app_rect(0,0,800,600,0);for(int y=0;y<h;y++)for(int x=0;x<w;x++)app_rect(ox+x*scale,oy+y*scale,scale,scale,src[y*w+x]);app_present();}
int app_key(int wait){int k;do{app_poll();if(closing){closing=0;return 27;}if(opening==1){opening=2;return KEY_OPEN;}k=key(0);if(!k&&wait){audio_pump();sleep(10);}}while(wait&&!k);return k;}
int app_event(void){app_poll();if(closing){closing=0;return KEY_F10;}if(opening==1){opening=2;return KEY_OPEN;}return keyevent();}
int app_opened_path(char*out,u32 cap){if(opening!=2||!out||strlen(openpath)+1>cap)return -1;strcpy(out,openpath);opening=0;return 0;}
static int filter_match(const char*name,const char*filter){if(!filter||!*filter)return 1;int n=strlen(name);const char*p=filter;while(*p){char e[12];int j=0;while(*p&&*p!=';'){if(j<11)e[j++]=*p;p++;}e[j]=0;if(n>=j&&!stricmp(name+n-j,e))return 1;if(*p)p++;}return 0;}
int file_dialog(const char*filter,char*out,u32 cap){
    if(!out||cap<2)return -1;int oldvol=fs_volume(),oldcwd=cwd,sel=0,buttons=mouse_buttons&1,first=1;u32 clicktime=0;char dir[4096];fs_pwd(dir);if(filter&&!stricmp(filter,".NES"))fs_cd("/NES");if(filter&&!stricmp(filter,".WAD"))fs_cd("/");
    serial("FILE MANAGER OPEN: ");serial(filter?filter:"all files");serial("\n");
    for(;;){int list[128],n=0;for(int i=0;i<128;i++)if(files[i].type&&files[i].parent==(u32)cwd&&(files[i].type==2||filter_match(files[i].name,filter)))list[n++]=i;if(first&&filter&&!stricmp(filter,".WAD"))for(int i=0;i<n;i++)if(!stricmp(files[list[i]].name,"FREEDOOM1.WAD"))sel=i;first=0;if(sel>=n)sel=n?n-1:0;
        desktop_background();int x=64,y=44,w=screen_w-128,h=screen_h-100;rect(x,y,w,h,0xc5c8c0);rect(x+4,y+4,w-8,26,0x36594b);icon_draw("FILEMAN",x+10,y+9,1);text(x+32,y+12,"File Manager - Open",0xffffff);text(x+12,y+42,"Enter open | Backspace parent | Tab/Left/Right drive | ESC",0x17211d);char path[4096];fs_pwd(path);char device[96];fs_volume_label(device);text(x+12,y+62,device,0x17211d);rect(x+w-112,y+56,44,24,0x416b58);rect(x+w-60,y+56,44,24,0x416b58);text(x+w-100,y+64,"<",0xffffff);text(x+w-48,y+64,">",0xffffff);
        int visible=(h-116)/20,first=sel>=visible?sel-visible+1:0;for(int j=first;j<n&&j<first+visible;j++){int yy=y+88+(j-first)*20;FsEntry*f=files+list[j];if(j==sel)rect(x+8,yy-2,w-16,20,0x416b58);icon_draw(f->type==2?"FOLDER":f->name,x+12,yy,1);text(x+34,yy+4,f->name,j==sel?0xffffff:0x17211d);if(f->type==2)text(x+w-72,yy+4,"<DIR>",j==sel?0xffffff:0x17211d);}text(x+12,y+h-20,path,0x17211d);cursor_draw();present();
        int k=key(0);while(keyevent());int down=mouse_buttons&1;if(down&&!buttons){int xx=mouse_x-x,yy=mouse_y-y;if(xx>w-28&&yy<32)k=27;else if(yy>=56&&yy<80&&xx>=w-112)k=xx<w-64?KEY_LEFT:KEY_RIGHT;else if(xx>=8&&xx<w-8&&yy>=86&&yy<h-28){int row=first+(yy-86)/20;if(row<n){if(row==sel&&ticks()-clicktime<350)k=13;sel=row;clicktime=ticks();}}}buttons=down;if(k==27)break;if(k==KEY_UP&&sel)sel--;if(k==KEY_DOWN&&sel+1<n)sel++;if(k==8){fs_cd("..");sel=0;}if((k==9||k==KEY_LEFT||k==KEY_RIGHT)&&volume_count){if(!fs_next_volume(k==KEY_LEFT?-1:1))sel=0;}
        if(k==13&&n){FsEntry*f=files+list[sel];if(f->type==2){fs_cd(f->name);sel=0;}else{fs_pwd(path);if(strcmp(path,"/"))strcpy(path+strlen(path),"/");if(strlen(path)+strlen(f->name)+1>cap){print("Selected path exceeds limit\n");continue;}strcpy(path+strlen(path),f->name);if(fs_qualify(fs_volume(),path,out,cap)){print("Selected path exceeds limit\n");continue;}if(fs_volume()!=oldvol)fs_mount(oldvol);cwd=oldcwd;serial("FILE MANAGER SELECTED: ");serial(out);serial("\n");return 0;}}sleep(20);
    }
    if(fs_volume()!=oldvol)fs_mount(oldvol);cwd=oldcwd;return -1;
}
int app_resource(const char*name,void*out,u32 cap){
    if(!current||!name||!out||!cap||strlen(name)>200)return -1;char path[256];if(name[0]=='/'){int v=atoi(title+1);if(fs_qualify(v,name,path,sizeof(path)))return -1;}else{strcpy(path,title);int j=strlen(path);while(j&&path[j-1]!='/')j--;path[j]=0;strcpy(path+j,name);}int f=fs_find(path),n=fs_size(f);if(n<0||(u32)n>cap)return -1;return fs_read(f,0,out,n)==n?n:-1;
}
