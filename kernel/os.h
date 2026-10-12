#ifndef OS_H
#define OS_H
#include "bds.h"
static inline void outb(u16 p,u8 v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline u8 inb(u16 p){u8 v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outw(u16 p,u16 v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static inline u16 inw(u16 p){u16 v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outl(u16 p,u32 v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}
static inline u32 inl(u16 p){u32 v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}
void *memcpy(void*,const void*,usize);void *memset(void*,int,usize);int memcmp(const void*,const void*,usize);
usize strlen(const char*);int strcmp(const char*,const char*);int stricmp(const char*,const char*);
void strcpy(char*,const char*);void number(char*,u32);int atoi(const char*);
void serial_init(void);void serial(const char*);void print(const char*);void clear_console(void);
int key(int);int keyevent(void);u32 ticks(void);void sleep(u32);void hardware_init(void);
void *alloc(usize);void dealloc(void*);void heap_reset(void);u32 heap_used(void);
extern int screen_w,screen_h,graphics;
void video_init(u32);void rect(int,int,int,int,u32);void text(int,int,const char*,u32);void present(void);void blit(const u32*,int,int);
int fs_init(void);int fs_find(const char*);int fs_read(int,u32,void*,u32);int fs_write(const char*,const void*,u32);int fs_size(int);
int fs_mkdir(const char*);int fs_remove(const char*);int fs_rename(const char*,const char*);int fs_copy(const char*,const char*);
int fs_volume(void);extern int fs_system_volume;
int fs_qualify(int,const char*,char*,u32);int fs_type(int);int fs_next_volume(int);void fs_volume_label(char*);
int run_system(const char*,int,char**);int app_pointer(int*,int*);void system_reset(void);void system_poweroff(void);
void fs_list(void);void fs_pwd(char*);int fs_cd(const char*);int fs_check(void);extern FsEntry files[128];extern int cwd;
int run(const char*,int,char**);void app_exit(int);void shell_command(char*);void shell(void);void desktop(int);
extern int mouse_x,mouse_y,mouse_buttons;void input_poll(void);
void desktop_background(void);
void icon_draw(const char*,int,int,int);void icons_reset(void);
void cursor_draw(void);
int font_load(const char*);void font_reset(void);int bmp_draw(const char*);void boot_splash(void);
void wallpaper_init(void);void wallpaper_draw(void);void video_stretch(const u32*,int,int);
void video_scaled(const u32*,int,int,int,int,int,int);
void system_sound(int);
void net_init(void);void net_poll(void);int net_info(char*,u32);int http_get(const char*,char*,u32);
void app_begin(BdsApi*,const char*);void app_end(void);void app_rect(int,int,int,int,u32);void app_text(int,int,const char*,u32);void app_present(void);void app_blit(const u32*,int,int);int app_key(int);int app_event(void);void app_fullscreen(int);void app_poll(void);
int file_dialog(const char*,char*,u32);int app_resource(const char*,void*,u32);
int app_opened_path(char*,u32);
void audio_init(void);int audio_available(void);int audio_write(const short*,u32,u32,int);void audio_stop(void);void audio_pump(void);void audio_mute(int);int audio_is_muted(void);int music_play(const char*,int);void music_stop(void);void music_mix(short*,u32);
int music_memory(const void*,u32,int);void music_volume(int);
#endif
