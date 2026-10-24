#include "os.h"
#include "storage.h"
extern void console_draw(int,int,int,int);
typedef struct{int open,x,y,w,h,minimized,maximized,oldx,oldy,oldw,oldh;}Window;
static Window win[7];static int focus,menu,theme,drag=-1,dx,dy;
int desktop_theme(int value){if(value>=0)theme=!!value;return theme;}
static const char*titles[]={"Terminal","File Manager","Text Editor","System Information","Game Center","Settings","About BDX-OS","Image Viewer","Web Browser","Media Player"};
static const char*icon_names[]={"TERMINAL","FILEMAN","EDITOR","SYSINFO","GAME","SETTINGS","ABOUT","IMAGE","BROWSER","PLAYER"};
static char cmd[256],dialog[128];static int cn,selected;
static void launch(const char*,int,char**);
static void openwin(int i){menu=0;if(i>=7){char*args[]={i==7?"IMAGE.RUN":i==8?"BROWSER.RUN":"PLAYER.RUN"};launch(i==7?"/IMAGE.RUN":i==8?"/BROWSER.RUN":"/PLAYER.RUN",1,args);return;}if(!win[i].open){win[i]=(Window){1,72+i*17,56+i*14,screen_w<700?screen_w-32:620,screen_h<520?screen_h-80:410};if(win[i].x+win[i].w>screen_w)win[i].x=screen_w-win[i].w;if(win[i].y+win[i].h>screen_h-36)win[i].y=screen_h-36-win[i].h;system_sound(1);}focus=i;win[i].minimized=0;if(i==5)settings_open();}
static void button(int x,int y,int w,int h,const char*s,int active){rect(x,y,w,h,active?0x405b52:0xbfc1bd);rect(x,y,w,1,0xffffff);rect(x,y,1,h,0xffffff);rect(x+w-1,y,1,h,0x393e3b);rect(x,y+h-1,w,1,0x393e3b);text(x+8,y+8,s,active?0xf2f3dd:0x171d1c);}
static void maximize(int i){Window*w=win+i;if(!w->open)return;if(w->maximized){w->x=w->oldx;w->y=w->oldy;w->w=w->oldw;w->h=w->oldh;}else{w->oldx=w->x;w->oldy=w->y;w->oldw=w->w;w->oldh=w->h;w->x=16;w->y=8;w->w=screen_w-32;w->h=screen_h-80;}w->maximized=!w->maximized;w->minimized=0;serial("WINDOW: maximize toggled\n");}
static int listing[128],listn;
static void contents(int i){Window*w=win+i;int x=w->x+12,y=w->y+40;
    if(i==0){rect(x-4,y-4,w->w-16,w->h-50,0x151e19);console_draw(x,y,23,73);text(x,y+w->h-76,cmd,0xffd58a);}
    else if(i==1){char p[4096];fs_pwd(p);text(x,y,p,0x1b2922);char label[96];fs_volume_label(label);text(x+180,y,label,0x1b2922);button(x+w->w-112,y-6,36,22,"<",0);button(x+w->w-70,y-6,36,22,">",0);listn=0;for(int j=0;j<128;j++)if(files[j].type&&files[j].parent==(u32)cwd)listing[listn++]=j;
        if(selected>=listn)selected=listn?listn-1:0;int first=selected>=16?selected-15:0;
        for(int row=first;row<listn&&row<first+16;row++){FsEntry*f=files+listing[row];int yy=y+24+(row-first)*20;if(row==selected)rect(x-3,yy,w->w-24,20,0x416b58);icon_draw(f->type==2?"FOLDER":f->name,x,yy+2,1);text(x+24,yy+6,f->name,row==selected?0xffffff:0x1b2922);if(f->type==2)text(x+304,yy+6,"<DIR>",0x1b2922);}
        text(x,w->y+w->h-24,"Up/Down open | Backspace parent | Left/Right drive",0x343b37);}
    else if(i==2){text(x,y,"Editor is a native .RUN application.",0x1b2922);text(x,y+24,"Press Enter to edit /NOTES.TXT",0x1b2922);text(x,y+48,"F2 saves. ESC returns to desktop.",0x1b2922);}
    else if(i==3){text(x,y,"BDX-OS ULTIMATE / " BDS_ARCH_NAME "",0x1b2922);text(x,y+32,"128 MiB QEMU profile | 64 MiB dynamic heap",0x1b2922);text(x,y+56,"BDSFS / FAT / ISO9660 storage",0x1b2922);text(x,y+80,"Native .RUN loader / fixed address",0x1b2922);text(x,y+104,"PIT timer, PS/2 input, RGB framebuffer",0x1b2922);text(x,y+144,"Cooperative apps share kernel privilege.",0x1b2922);text(x,y+168,"BIOS storage: HDD, USB and optical media.",0x1b2922);}
    else if(i==4){const char*s[]={"1  Snake","2  Pong","3  Falling Blocks","4  Mines","5  Star Arcade","D  DOOM / Freedoom","N  NES Emulator","C  ClassiCube","P  2048"};for(int j=0;j<9;j++)button(x,y+j*36,340,32,s[j],0);text(x,y+332,"ClassiCube: WASD, mouse, Esc menu, F10 exit",0x1b2922);}
    else if(i==5)settings_draw(x,y);
    else {text(x,y,"BDX-OS ULTIMATE",0x1b2922);text(x,y+32,"A hobby kernel, booted through GRUB 2.",0x1b2922);text(x,y+64,"8086 floppy edition is a separate kernel.",0x1b2922);text(x,y+104,"DOOM: id Software / doomgeneric, GPL v2.",0x1b2922);text(x,y+128,"Freedoom game data: BSD license.",0x1b2922);}
}
static void frame(int i){Window*w=win+i;rect(w->x+4,w->y+4,w->w,w->h,0x111815);rect(w->x,w->y,w->w,w->h,0xc5c8c0);rect(w->x,w->y,w->w,2,0xffffff);rect(w->x,w->y,2,w->h,0xffffff);rect(w->x+5,w->y+5,w->w-10,25,i==focus?WINDOW_TITLE_COLOR:0x747e74);icon_draw(icon_names[i],w->x+10,w->y+9,1);text(w->x+32,w->y+13,titles[i],0xffffff);window_control(w->x+w->w-28,w->y+7,0);window_control(w->x+w->w-52,w->y+7,2);window_control(w->x+w->w-76,w->y+7,1);contents(i);}
static void draw(int show_cursor){if(theme)rect(0,0,screen_w,screen_h,0x2b2921);else wallpaper_draw();text(screen_w-220,30,"BDX-OS / DESKTOP",theme?0xebc36c:0x203c54);text(screen_w-220,48,"native desktop   v0.4",0x294757);
    for(int i=0;i<7;i++){int y=28+i*((screen_h-90)/7<67?(screen_h-90)/7:67);icon_draw(icon_names[i],22,y,2);text(10,y+37,titles[i],theme?0xd6ddcf:0x172a39);}
    for(int i=7;i<10;i++){int y=100+(i-7)*85;icon_draw(icon_names[i],screen_w-78,y,2);text(screen_w-112,y+38,titles[i],0x172a39);}
    for(int i=0;i<7;i++)if(win[i].open&&!win[i].minimized&&i!=focus)frame(i);if(win[focus].open&&!win[focus].minimized)frame(focus);
    rect(0,screen_h-36,screen_w,36,0xaab1a5);button(4,screen_h-32,88,28,"",menu);icon_draw("START",10,screen_h-26,1);text(32,screen_h-24,"Start",0x171d1c);int tx=100;for(int i=0;i<7;i++)if(win[i].open){button(tx,screen_h-32,92,28,titles[i],focus==i);tx+=96;}
    if(tx<screen_w-150){int d[6];input_date_time(d);char clock[12]={'0'+d[3]/10,'0'+d[3]%10,':','0'+d[4]/10,'0'+d[4]%10,' ','U','T','C',0};text(screen_w-104,screen_h-23,clock,0x172531);}
    if(menu){rect(4,screen_h-432,258,396,0xc5c8c0);for(int i=0;i<12;i++){int y=screen_h-426+i*32;icon_draw(i<10?icon_names[i]:i==10?"RESET":"POWER",14,y,1);text(40,y+4,i<10?titles[i]:i==10?"Restart    [R]":"Power off  [P]",0x202b27);}}
    if(dialog[0]){rect(190,220,440,100,0xc5c8c0);text(206,238,"Application error",0x7a312a);text(206,264,dialog,0x222222);text(206,290,"Press Enter to dismiss",0x222222);}
    if(show_cursor)cursor_draw();}
void desktop_background(void){draw(0);}
static void launch(const char*path,int n,char**args){if((path[0]=='/'?run_system(path,n,args):run(path,n,args))<0){strcpy(dialog,"Launch failed. See terminal for details.");system_sound(2);}}
static void game_launch(int index){const char*names[]={"snake","pong","blocks","mines","arcade"};if(index>=0&&index<5){char*a[]={"GAME.RUN",(char*)names[index]};launch("/GAME.RUN",2,a);}else if(index==8){char*a[]={"PUZZLE.RUN"};launch("/PUZZLE.RUN",1,a);}else if(index==7){char*a[]={"CUBE.RUN"};launch("/CUBE.RUN",1,a);}else if(index==6){char*a[]={"NES.RUN"};launch("/NES.RUN",1,a);}else{char*a[]={"DOOM.RUN"};launch("/DOOM.RUN",1,a);}}
static int point_window(int x,int y){for(int i=0;i<7;i++)if(win[i].open&&!win[i].minimized&&x>=win[i].x&&x<win[i].x+win[i].w&&y>=win[i].y&&y<win[i].y+win[i].h)return 1;return 0;}
void desktop(int initial){if(!graphics)return;openwin(initial);serial("DESKTOP ready\n");int prev=0,lastx=-1,lasty=-1;u32 lastpaint=0;for(;;){settings_poll();input_poll();int pressed=(mouse_buttons&1)&&!prev,released=!(mouse_buttons&1)&&prev;prev=mouse_buttons&1;
    if(pressed&&settings_pending()){settings_click(mouse_x-win[5].x-12,mouse_y-win[5].y-40);}
    else if(pressed){system_sound(1);if(mouse_y>=screen_h-36){if(mouse_x<96)menu=!menu;else{int tx=100;for(int i=0;i<7;i++)if(win[i].open){if(mouse_x>=tx&&mouse_x<tx+92){win[i].minimized=focus==i&&!win[i].minimized;focus=i;}tx+=96;}}}
        else if(menu&&mouse_x<262&&mouse_y>=screen_h-432){int i=(mouse_y-(screen_h-426))/32;if(i<10)openwin(i);else if(i==10)system_reset();else if(i==11)system_poweroff();}
        else if(mouse_x>=screen_w-112&&mouse_y>=100&&mouse_y<355&&!point_window(mouse_x,mouse_y))openwin(7+(mouse_y-100)/85);
        else if(mouse_x<95&&!point_window(mouse_x,mouse_y)){int i=(mouse_y-28)/((screen_h-90)/7<67?(screen_h-90)/7:67);if(i>=0&&i<7)openwin(i);}
        else{for(int step=0;step<8;step++){int i=step==0?focus:7-step;if(i<0||i>6||!win[i].open||win[i].minimized)continue;Window*w=win+i;if(mouse_x>=w->x&&mouse_x<w->x+w->w&&mouse_y>=w->y&&mouse_y<w->y+w->h){focus=i;if(mouse_y<w->y+32){if(mouse_x>w->x+w->w-32)w->open=0;else if(mouse_x>w->x+w->w-56)maximize(i);else if(mouse_x>w->x+w->w-80){w->minimized=1;serial("WINDOW: minimized\n");}else{drag=i;dx=mouse_x-w->x;dy=mouse_y-w->y;}}else if(i==1&&mouse_y<w->y+56&&mouse_x>=w->x+w->w-100){if(!fs_next_volume(mouse_x<w->x+w->w-58?-1:1))selected=0;}else if(i==5)settings_click(mouse_x-w->x-12,mouse_y-w->y-40);else if(i==4){int g=(mouse_y-w->y-40)/36;if(g>=0&&g<9)game_launch(g);}break;}}}}
    if(released||settings_pending())drag=-1;if(drag>=0){win[drag].x=mouse_x-dx;win[drag].y=mouse_y-dy;}
    int k=key(0);if(k){if(settings_pending())settings_key(k);else if(dialog[0]){if(k==13)dialog[0]=0;}
        else if(k>=KEY_MOVE_UP&&k<=KEY_MOVE_RIGHT){if(k==KEY_MOVE_UP)win[focus].y-=8;if(k==KEY_MOVE_DOWN)win[focus].y+=8;if(k==KEY_MOVE_LEFT)win[focus].x-=8;if(k==KEY_MOVE_RIGHT)win[focus].x+=8;}
        else if(k==KEY_F1+10)maximize(focus);else if(k==KEY_F1+11){win[focus].minimized=1;serial("WINDOW: minimized\n");}
        else if(k>=KEY_F1&&k<KEY_F1+9)openwin(k-KEY_F1);
        else if(k==KEY_F10)menu=!menu;
        else if(menu&&(k=='m'||k=='M'))openwin(9);
        else if(menu&&(k=='r'||k=='R'))system_reset();
        else if(menu&&(k=='p'||k=='P'))system_poweroff();
        else if(menu&&k>='1'&&k<='9')openwin(k-'1');
        else if(k==9&&focus==5&&win[5].open)settings_key(k);
        else if(k==9){for(int j=1;j<=7;j++)if(win[(focus+j)%7].open){focus=(focus+j)%7;win[focus].minimized=0;break;}}
        else if(k==27){if(menu)menu=0;else win[focus].open=0;}
        else if(focus==0&&win[0].open){if(k==13){cmd[cn]=0;print("BDX> ");print(cmd);print("\n");shell_command(cmd);cn=0;cmd[0]=0;}else if(k==8){if(cn)cmd[--cn]=0;}else if(k>=32&&k<127&&cn<255){cmd[cn++]=k;cmd[cn]=0;}}
        else if(focus==1){if((k==KEY_LEFT||k==KEY_RIGHT)&&volume_count){if(!fs_next_volume(k==KEY_LEFT?-1:1))selected=0;}if(k==KEY_UP&&selected)selected--;if(k==KEY_DOWN&&selected+1<listn)selected++;if(k==8){fs_cd("..");selected=0;}if(k==13&&selected<listn){FsEntry*f=files+listing[selected];if(f->type==2){fs_cd(f->name);selected=0;}else{char p[4096];fs_pwd(p);if(strcmp(p,"/"))strcpy(p+strlen(p),"/");strcpy(p+strlen(p),f->name);char qualified[4096];if(!fs_qualify(fs_volume(),p,qualified,sizeof(qualified)))strcpy(p,qualified);int n=strlen(f->name);if(n>4&&!stricmp(f->name+n-4,".RUN")){char*a[]={p};launch(p,1,a);}else if(n>4&&!stricmp(f->name+n-4,".WAD")){char*a[]={"DOOM.RUN","-iwad",p};launch("/DOOM.RUN",3,a);}else if(n>4&&(!stricmp(f->name+n-4,".BMP")||!stricmp(f->name+n-4,".PNG"))){char*a[]={"IMAGE.RUN",p};launch("/IMAGE.RUN",2,a);}else if((n>4&&!stricmp(f->name+n-4,".HTM"))||(n>5&&!stricmp(f->name+n-5,".HTML"))){char*a[]={"BROWSER.RUN",p};launch("/BROWSER.RUN",2,a);}else if(n>4&&!stricmp(f->name+n-4,".NES")){char*a[]={"NES.RUN",p};launch("/NES.RUN",2,a);}else if((n>5&&(!stricmp(f->name+n-5,".MPEG")||!stricmp(f->name+n-5,".MIDI")))||(n>4&&(!stricmp(f->name+n-4,".MP3")||!stricmp(f->name+n-4,".MID")||!stricmp(f->name+n-4,".WAV")||!stricmp(f->name+n-4,".MPG")||!stricmp(f->name+n-4,".VOB")||!stricmp(f->name+n-4,".M2V")||!stricmp(f->name+n-4,".CUE")))){char*a[]={"PLAYER.RUN",p};launch("/PLAYER.RUN",2,a);}else if(n>4&&(!stricmp(f->name+n-4,".PSF")||!stricmp(f->name+n-4,".TTF"))){if(font_load(p))strcpy(dialog,"Invalid PSF/TrueType custom font.");}else if(n>4&&(!stricmp(f->name+n-4,".BIN")||!stricmp(f->name+n-4,".DLL")||!stricmp(f->name+n-4,".DCL"))){strcpy(dialog,"Companion data file; load using the app resource API.");}else{char*a[]={"EDITOR.RUN",p};launch("/EDITOR.RUN",2,a);}}}}
        else if(focus==2&&k==13){char*a[]={"EDITOR.RUN","/NOTES.TXT"};launch("/EDITOR.RUN",2,a);}
        else if(focus==4){if(k>='1'&&k<='5')game_launch(k-'1');else if(k=='d'||k=='D')game_launch(5);else if(k=='n'||k=='N')game_launch(6);else if(k=='c'||k=='C')game_launch(7);else if(k=='p'||k=='P')game_launch(8);}
        else if(focus==5&&win[5].open){settings_key(k);}
        else {if(k==KEY_LEFT)win[focus].x-=8;if(k==KEY_RIGHT)win[focus].x+=8;if(k==KEY_UP)win[focus].y-=8;if(k==KEY_DOWN)win[focus].y+=8;}}
    for(int i=0;i<7;i++){Window*w=win+i;if(w->w>screen_w-32)w->w=screen_w-32;if(w->h>screen_h-80)w->h=screen_h-80;if(w->x+w->w>screen_w)w->x=screen_w-w->w;if(w->y+w->h>screen_h-36)w->y=screen_h-36-w->h;if(w->x<0)w->x=0;if(w->y<0)w->y=0;}
    if(settings_pending()){focus=5;menu=0;}
    if(k||pressed||released||mouse_x!=lastx||mouse_y!=lasty||settings_pending()||ticks()-lastpaint>=1000){draw(1);present();lastx=mouse_x;lasty=mouse_y;lastpaint=ticks();}sleep(10);
}}
