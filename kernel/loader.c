#include "os.h"
#include "storage.h"
extern int call_app(void*,BdsApi*,int,char**);
static BdsApi api;
int run(const char*path,int argc,char**argv){
    if(argc<0||argc>16)return -1;char apppath[256];if(fs_qualify(fs_volume(),path,apppath,sizeof(apppath)))return -1;
    char restart_path[256];char*restart_args[]={"DOOM.RUN","-iwad",restart_path};
retry:
    int f=fs_find(path),size=fs_size(f);RunHeader h;
    if(size<32||fs_read(f,0,&h,32)!=32){print("RUN: file missing or too short\n");return -1;}
    if(memcmp(h.magic,"BDSR",4)||(h.version!=1&&h.version!=2)||h.arch!=BDS_RUN_ARCH||(h.version==1&&h.flags)||(h.version==2&&(h.flags<22||h.flags>BDS_ICON_BYTES))||h.code>(u32)size-32||h.flags!=(u32)size-32-h.code||h.entry>=h.code||h.memory<h.code+h.bss||h.memory>0x800000||h.code>0x800000||h.bss>0x800000-h.code){print("RUN: invalid or incompatible header\n");return -1;}
    u8*base=(u8*)0x800000;memset(base,0,h.memory);if(fs_read(f,32,base,h.code)!=(int)h.code){print("RUN: read error\n");return -1;}u32 sum=0;for(u32 i=0;i<h.code;i++)sum=sum*31+base[i];u8 icon[BDS_ICON_BYTES];if(h.flags){if(fs_read(f,32+h.code,icon,h.flags)!=(int)h.flags)return -1;for(u32 i=0;i<h.flags;i++)sum=sum*31+icon[i];}if(sum!=h.checksum){print("RUN: checksum failed\n");return -1;}
    api=(BdsApi){7,print,app_key,ticks,sleep,alloc,dealloc,fs_find,fs_read,fs_write,fs_size,app_rect,app_text,app_present,app_blit,app_exit,800,600,app_event,fs_remove,fs_mkdir,fs_rename,file_dialog,font_load,audio_write,music_play,audio_stop,audio_available,app_fullscreen,app_resource,app_opened_path,music_memory,music_volume,net_info,http_get,app_pointer,fs_readdir,app_mouse_capture,input_relative,input_modifiers,input_date_time,app_surface_blit,app_viewport,music_control,cd_count,cd_info,cd_read,audio_pause,audio_pending,clipboard_set,clipboard_get,system_info,app_frame_submit};
    print("RUN loaded: ");print(path);print("\n");while(key(0));while(keyevent());music_stop();audio_stop();heap_reset();int oldvol=fs_volume(),oldcwd=cwd;char argument[16][256];char*args[16];for(int i=0;i<argc;i++){args[i]=argv[i];if(i&&(oldvol!=f/128-1||argv[i][0]!='/')&&fs_find(argv[i])>=0&&!fs_qualify(oldvol,argv[i],argument[i],256))args[i]=argument[i];}if(fs_mount(f/128-1))return -1;app_begin(&api,apppath);int status=call_app(base+h.entry,&api,argc,args);int restart=status==42&&!app_opened_path(restart_path,sizeof(restart_path));audio_stop();music_stop();app_end();heap_reset();if(fs_volume()!=oldvol)fs_mount(oldvol);cwd=oldcwd;while(key(0));while(keyevent());
    if(restart){argc=3;argv=restart_args;goto retry;}
    print("RUN returned: ");char n[16];number(n,status);print(n);print("\n");return status;
}

int run_system(const char*path,int argc,char**argv){char p[256];if(fs_qualify(fs_system_volume,path,p,sizeof(p)))return -1;return run(p,argc,argv);}
