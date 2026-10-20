#include "os.h"
static int selected,previous=-1,tab,font_choice=1;static u32 deadline;static char status[96];
static const char*tabs[]={"Display","Colors","Fonts","Sounds"};
int settings_pending(void){return previous>=0;}
static int path(char*out,const char*name){return fs_qualify(fs_system_volume,name,out,256);}
static int load_font(int choice){char p[256];if(!choice){font_reset();return 0;}if(path(p,choice==1?"/FONTS/DEFAULT.PSF":choice==2?"/FONTS/BOLD.PSF":"/FONTS/USER.PSF"))return -1;return font_load(p);}
static void save(void){
    char p[256];u8 b[16]={'B','D','P','1',desktop_theme(-1),audio_volume(-1),audio_is_muted(),system_sounds(-1),font_choice};
    if(path(p,"/SETTINGS.CFG")||fs_write(p,b,sizeof(b)))strcpy(status,"Changed for this session; system disk could not save it.");
    else{strcpy(status,"Preferences saved on the system disk.");serial("SETTINGS: preferences saved\n");}
}
void settings_init(void){
    char p[256];u8 b[16];if(path(p,"/SETTINGS.CFG"))return;int f=fs_find(p);
    if(fs_size(f)!=16||fs_read(f,0,b,16)!=16||memcmp(b,"BDP1",4)||b[4]>1||b[5]>100||b[6]>1||b[7]>1||b[8]>3)return;
    desktop_theme(b[4]);audio_volume(b[5]);audio_mute(b[6]);system_sounds(b[7]);if(!load_font(b[8]))font_choice=b[8];serial("SETTINGS: preferences restored\n");
}
void settings_open(void){if(!settings_pending()){tab=0;selected=display_current();strcpy(status,"Tab changes category. Arrows select; Enter previews.");}}
static void control(int x,int y,int w,const char*label,int active){rect(x,y,w,24,active?0x405b52:0xbfc1bd);rect(x,y,w,1,0xffffff);rect(x,y,1,24,0xffffff);rect(x+w-1,y,1,24,0x393e3b);rect(x,y+23,w,1,0x393e3b);text(x+8,y+8,label,active?0xffffff:0x171d1c);}
static void dimensions(char*s,int w,int h){char n[16];number(s,w);strcpy(s+strlen(s)," x ");number(n,h);strcpy(s+strlen(s),n);}
void settings_draw(int x,int y){
    char label[80],size[32];for(int i=0;i<4;i++)control(x+i*130,y,120,tabs[i],tab==i);
    if(tab==0){
        strcpy(label,"Current: ");dimensions(size,screen_w,screen_h);strcpy(label+strlen(label),size);strcpy(label+strlen(label)," (32-bit color)");text(x,y+36,label,0x1b2922);
        int first=selected<0?0:selected/8*8;
        for(int i=first;i<display_count&&i<first+8;i++){int w=display_modes[i].w,h=display_modes[i].h;dimensions(label,w,h);if(w*10==h*16)strcpy(label+strlen(label)," 16:10");else if(w*9==h*16||w==1360||w==1366)strcpy(label+strlen(label)," 16:9");else if(w*3>h*5)strcpy(label+strlen(label)," Wide");control(x+((i-first)%2)*260,y+72+((i-first)/2)*28,240,label,i==selected);}
        if(display_count>8){control(x,y+188,140,"< Previous",0);control(x+156,y+188,140,"Next >",0);strcpy(label,"Page ");number(size,first/8+1);strcpy(label+strlen(label),size);text(x+312,y+196,label,0x1b2922);}
        if(settings_pending()){
            control(x,y+220,140,"Keep [Enter]",1);control(x+156,y+220,140,"Revert [Esc]",0);
            strcpy(label,"Reverting in ");number(size,(deadline-ticks()+999)/1000);strcpy(label+strlen(label),size);strcpy(label+strlen(label)," seconds");text(x,y+258,label,0x7a312a);
        }else{if(display_count)control(x,y+220,140,"Apply [Enter]",0);text(x,y+258,display_count?"Firmware-supported sizes only. Keep saves the size.":"Live switching unavailable; use the GRUB boot menu.",0x1b2922);}
        text(x,y+310,"Fullscreen apps keep their shape, with borders if needed.",0x1b2922);
    }else if(tab==1){
        text(x,y+44,"Desktop colors / wallpaper",0x1b2922);control(x,y+76,240,"Cloud wallpaper [1]",!desktop_theme(-1));control(x+260,y+76,240,"Amber background [2]",desktop_theme(-1));
        text(x,y+124,"RGB32: 16.7 million colors, 8 bits per color channel.",0x1b2922);text(x,y+152,"Wallpaper fills the screen with proportional cropping.",0x1b2922);
    }else if(tab==2){
        text(x,y+44,"Fixed-width desktop and native application font",0x1b2922);
        control(x,y+76,240,"Built-in font [B]",font_choice==0);control(x+260,y+76,240,"Default PSF [D]",font_choice==1);
        control(x,y+112,240,"Bold PSF [H]",font_choice==2);control(x+260,y+112,240,"Choose PSF file [O]",font_choice==3);
        text(x,y+164,"Preview: Aa Bb Cc 0123456789 !?",0x1b2922);text(x,y+192,"PSF1 glyphs are fitted to the desktop's 8 x 8 cells.",0x1b2922);
    }else{
        text(x,y+44,audio_available()?"Audio output: supported controller ready":"Audio output: no supported controller detected",0x1b2922);
        control(x,y+76,240,audio_is_muted()?"Unmute [M]":"Mute [M]",audio_is_muted());control(x+260,y+76,240,system_sounds(-1)?"System sounds: on [C]":"System sounds: off [C]",system_sounds(-1));
        control(x,y+112,110,"- Volume",0);control(x+126,y+112,110,"+ Volume",0);strcpy(label,"Master: ");number(size,audio_volume(-1));strcpy(label+strlen(label),size);strcpy(label+strlen(label),"%");text(x+260,y+120,label,0x1b2922);
        control(x,y+156,240,"Test startup sound [T]",0);text(x,y+200,"Master volume affects games, music and system sounds.",0x1b2922);
    }
    text(x,y+284,status,0x1b2922);
}
static void revert(void){if(previous<0)return;int old=previous;previous=-1;if(display_set(old))strcpy(status,"Restore failed. Reboot using the GRUB recovery entry.");else{selected=old;strcpy(status,"Previous resolution restored. Saved setting unchanged.");serial("DISPLAY: preview reverted\n");}}
static void apply(void){if(previous>=0||selected<0)return;int old=display_current();if(display_set(selected)){strcpy(status,"Firmware rejected this mode. Previous size retained.");return;}previous=old;deadline=ticks()+15000;strcpy(status,"Keep this resolution? It will revert unless confirmed.");serial("DISPLAY: preview started\n");}
static void keep(void){if(previous<0)return;previous=-1;if(display_save())strcpy(status,"Kept for this session; system disk could not save it.");else strcpy(status,"Resolution kept and saved on the system disk.");serial("DISPLAY: preview kept\n");}
void settings_poll(void){if(previous>=0&&(int)(ticks()-deadline)>=0)revert();}
static void category(int i){tab=i;strcpy(status,"Tab changes category. Preferences save on the system disk.");serial("SETTINGS: tab ");serial(tabs[tab]);serial("\n");}
static void choose_font(int choice){
    if(choice==3){char p[256],dest[256];if(file_dialog(".PSF",p,sizeof(p)))return;
        if(font_load(p)){strcpy(status,"Invalid PSF1 font. The previous font was retained.");return;}
        if(path(dest,"/FONTS/USER.PSF")||fs_copy(p,dest)){strcpy(status,"Font loaded for this session; could not save a copy.");return;}
    }else if(load_font(choice)){strcpy(status,"Could not load the selected font from the system disk.");return;}
    font_choice=choice;save();
}
void settings_key(int k){
    if(previous>=0){if(k==13||k=='k'||k=='K')keep();else if(k==27||k=='r'||k=='R')revert();return;}
    if(k==9){category((tab+1)%4);return;}if(k>='A'&&k<='Z')k+=32;
    if((tab==0||tab==1)&&(k=='1'||k=='2')){desktop_theme(k-'1');save();return;}
    if(tab==0&&display_count){if(k==KEY_RIGHT||k==KEY_DOWN)selected=(selected+1)%display_count;else if(k==KEY_LEFT||k==KEY_UP)selected=(selected+display_count-1)%display_count;else if(k==13||k=='a')apply();}
    else if(tab==2){if(k=='b')choose_font(0);if(k=='d')choose_font(1);if(k=='h')choose_font(2);if(k=='o')choose_font(3);}
    else if(tab==3){if(k=='t'){system_sound(0);return;}if(k=='m')audio_mute(!audio_is_muted());else if(k=='c')system_sounds(!system_sounds(-1));else if(k=='-'||k==KEY_LEFT)audio_volume(audio_volume(-1)>10?audio_volume(-1)-10:0);else if(k=='+'||k=='='||k==KEY_RIGHT)audio_volume(audio_volume(-1)+10);else return;save();}
}
void settings_click(int x,int y){
    if(previous>=0){if(y>=220&&y<244){if(x>=0&&x<140)keep();else if(x>=156&&x<296)revert();}return;}
    if(x<0||y<0)return;if(y<24&&x<510&&x%130<120){category(x/130);return;}
    if(tab==0){if(y>=72&&y<180){int col=x/260,row=(y-72)/28,i=(selected<0?0:selected/8*8)+row*2+col;if(col<2&&x%260<240&&(y-72)%28<24&&i<display_count)selected=i;}
        else if(y>=188&&y<212&&display_count>8){int first=selected/8*8;if(x<140)selected=first>=8?first-8:0;else if(x>=156&&x<296)selected=first+8<display_count?first+8:display_count-1;}
        else if(y>=220&&y<244&&x<140)apply();
    }else if(tab==1&&y>=76&&y<100){if(x<240)settings_key('1');else if(x>=260&&x<500)settings_key('2');}
    else if(tab==2){if(y>=76&&y<100){if(x<240)choose_font(0);else if(x>=260&&x<500)choose_font(1);}else if(y>=112&&y<136){if(x<240)choose_font(2);else if(x>=260&&x<500)choose_font(3);}}
    else if(tab==3){if(y>=76&&y<100){if(x<240)settings_key('m');else if(x>=260&&x<500)settings_key('c');}else if(y>=112&&y<136){if(x<110)settings_key('-');else if(x>=126&&x<236)settings_key('+');}else if(y>=156&&y<180&&x<240)settings_key('t');}
}
