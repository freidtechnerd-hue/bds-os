#include "os.h"
#include "storage.h"
extern void console_full(void);
static void value(u32 v){char n[16];number(n,v);print(n);}
static void error(void){print("Operation failed: check path, type, free space, or mounted disk.\n");}
static u8 rtc(u8 r){outb(0x70,r);return inb(0x71);}
static int rtcval(u8 r){u8 v=rtc(r);return rtc(0x0b)&4?v:(v>>4)*10+(v&15);}
static void two(int v){char s[3]={'0'+v/10,'0'+v%10,0};print(s);}
static int parse(char*line,char**args){int n=0;char*s=line;while(*s&&n<12){while(*s==' ')s++;if(!*s)break;int quote=*s=='"';if(quote)s++;args[n++]=s;while(*s&&(quote?*s!='"':*s!=' '))s++;if(*s)*s++=0;}return n;}
void shell_command(char*line){
    char*a[12];int n=parse(line,a);if(!n)return;char*c=a[0];
    if(!stricmp(c,"HELP"))print("HELP VER CLS ECHO DIR CD PWD TYPE COPY MOVE DEL MKDIR RMDIR REN\nDATE TIME MEM SYSINFO RUN EDIT GAME DOOM NES IMAGE WEB NET PLAY FONT AUDIO DRIVES MOUNT REBOOT POWEROFF DESKTOP\nWRITE path \"text\" writes a file; CHECK validates disk metadata.\n.RUN apps: HELLO CALC EDITOR SYSINFO GAME DOOM NES PLAYER (i386 only).\nNES [path.nes] plays a cartridge or opens File Manager for a ROM.\nPaths support /, .. and quoted arguments. F1 opens desktop.\n");
    else if(!stricmp(c,"DRIVES"))storage_list();
    else if(!stricmp(c,"MOUNT")&&n==2){if(fs_mount(atoi(a[1])))error();else{print("Volume mounted. Current root: /\n");fs_list();}}
    else if(!stricmp(c,"VER"))print("BDS-OS Ultimate 0.2 / i386 native kernel\n");
    else if(!stricmp(c,"CLS"))clear_console();
    else if(!stricmp(c,"ECHO")){for(int i=1;i<n;i++){print(a[i]);if(i+1<n)print(" ");}print("\n");}
    else if(!stricmp(c,"DIR"))fs_list();
    else if(!stricmp(c,"PWD")){char p[4096];fs_pwd(p);print(p);print("\n");}
    else if(!stricmp(c,"CD")&&n==2){if(fs_cd(a[1]))error();}
    else if(!stricmp(c,"TYPE")&&n==2){int f=fs_find(a[1]),sz=fs_size(f);if(sz<0)error();else{char buf[513];u32 pos=0;while(pos<(u32)sz){int r=fs_read(f,pos,buf,512);if(r<=0){error();break;}buf[r]=0;print(buf);pos+=r;}print("\n");}}
    else if(!stricmp(c,"WRITE")&&n==3){if(fs_write(a[1],a[2],strlen(a[2])))error();else print("File written and flushed.\n");}
    else if(!stricmp(c,"COPY")&&n==3){if(fs_copy(a[1],a[2]))error();else print("File copied.\n");}
    else if((!stricmp(c,"MOVE")||!stricmp(c,"REN"))&&n==3){if(fs_rename(a[1],a[2]))error();else print("Entry moved.\n");}
    else if((!stricmp(c,"DEL")||!stricmp(c,"RMDIR"))&&n==2){int f=fs_find(a[1]);int type=!stricmp(c,"DEL")?1:2;if(f<0||fs_type(f)!=type||fs_remove(a[1]))error();else print("Entry removed.\n");}
    else if(!stricmp(c,"MKDIR")&&n==2){if(fs_mkdir(a[1]))error();else print("Directory created.\n");}
    else if(!stricmp(c,"DATE")){print("20");two(rtcval(9));print("-");two(rtcval(8));print("-");two(rtcval(7));print("\n");}
    else if(!stricmp(c,"TIME")){two(rtcval(4));print(":");two(rtcval(2));print(":");two(rtcval(0));print(" UTC (QEMU RTC)\n");}
    else if(!stricmp(c,"MEM")){print("Heap: 64 MiB; used ");value(heap_used());print(" bytes. App slot: 8 MiB.\n");}
    else if(!stricmp(c,"SYSINFO")){print("CPU: i386 protected mode, single cooperative ring-0 process\nPIT 100 Hz; PS/2 keyboard/mouse; COM1; BIOS EDD multi-drive storage\nVideo: ");value(screen_w);print("x");value(screen_h);print(graphics?" RGB32\n":" text fallback\n");print("Uptime ms: ");value(ticks());print("\n");}
    else if(!stricmp(c,"CHECK")){print(fs_check()?"FAIL: filesystem integrity\n":"PASS: filesystem integrity\n");}
    else if(!stricmp(c,"RUN")&&n>=2)run(a[1],n-1,a+1);
    else if(!stricmp(c,"EDIT")&&n==2){char*args[]={"EDITOR.RUN",a[1]};run_system("/EDITOR.RUN",2,args);}
    else if(!stricmp(c,"GAME")){if(n==1)print("GAME snake | pong | blocks | mines | arcade\nArrow keys, Space, R restart, ESC exit. Native GAME.RUN.\n");else{char*args[]={"GAME.RUN",a[1]};run_system("/GAME.RUN",2,args);}}
    else if(!stricmp(c,"DOOM")){char*args[]={"DOOM.RUN","-iwad",n>1?a[1]:0};run_system("/DOOM.RUN",n>1?3:1,args);}
    else if(!stricmp(c,"IMAGE")&&n<=2){char*args[]={"IMAGE.RUN",n>1?a[1]:0};run_system("/IMAGE.RUN",n>1?2:1,args);}
    else if(!stricmp(c,"WEB")&&n<=2){char*args[]={"BROWSER.RUN",n>1?a[1]:0};run_system("/BROWSER.RUN",n>1?2:1,args);}
    else if(!stricmp(c,"NET")){char info[180];net_info(info,sizeof(info));print(info);print("\n");}
    else if(!stricmp(c,"PLAY")&&n==2){char*args[]={"PLAYER.RUN",a[1]};run_system("/PLAYER.RUN",2,args);}
    else if(!stricmp(c,"FONT")&&n==2){if(!stricmp(a[1],"DEFAULT")){font_reset();print("Built-in font restored\n");}else if(font_load(a[1]))print("FONT: invalid PSF1 font\n");else print("Custom font loaded\n");}
    else if(!stricmp(c,"AUDIO")){print(audio_available()?"AC'97 / HDA stereo PCM output available\n":"No supported audio controller detected\n");}
    else if(!stricmp(c,"NES")&&n<=2){char*args[]={"NES.RUN",n>1?a[1]:0};run_system("/NES.RUN",n>1?2:1,args);}
    else if(!stricmp(c,"DESKTOP")){if(graphics)desktop(0);else print("No compatible RGB framebuffer.\n");}
    else if(!stricmp(c,"POWEROFF"))system_poweroff();
    else if(!stricmp(c,"REBOOT"))system_reset();
    else print("Unknown command or wrong arguments. Use HELP.\n");
}
void shell(void){print("BDS command line ready. HELP lists commands.\n");for(;;){char line[256];int n=0;print("BDS> ");console_full();for(;;){int k=key(1);if(k==KEY_F1&&graphics){desktop(0);continue;}if(k==13){line[n]=0;print("\n");break;}if(k==8){if(n){n--;print("\b");}}else if(k>=32&&k<127&&n<255){line[n++]=k;char s[2]={k,0};print(s);}console_full();}shell_command(line);}}
