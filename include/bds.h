#ifndef BDS_H
#define BDS_H
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef __SIZE_TYPE__ usize;
#ifdef __x86_64__
#define BDS_RUN_ARCH 3
#define BDS_ARCH_NAME "x86-64"
#else
#define BDS_RUN_ARCH 2
#define BDS_ARCH_NAME "i386"
#endif
#define KEY_UP 256
#define KEY_DOWN 257
#define KEY_LEFT 258
#define KEY_RIGHT 259
#define KEY_F1 260
#define KEY_F10 269
#define KEY_RELEASE 0x10000
#define KEY_MOVE_UP 512
#define KEY_MOVE_DOWN 513
#define KEY_MOVE_LEFT 514
#define KEY_MOVE_RIGHT 515
#define KEY_FULLSCREEN 516
#define KEY_MENU 517
#define KEY_OPEN 518
#define KEY_ADDRESS 519
#define KEY_COPY 520
#define KEY_PASTE 521
#define KEY_CUT 522
#define KEY_SELECT_ALL 523
typedef struct {u32 bytes,abi,arch_bits,uptime_ms,heap_used,heap_total,screen_width,screen_height,frames,last_frame_ms;} BdsSystemInfo;
typedef struct {u32 lba;u8 number,control;u16 reserved;} CdTrack;
typedef struct {char model[41];u32 count,leadout;CdTrack tracks[99];} CdInfo;
typedef struct {
    u32 version;
    void (*print)(const char*);
    int (*key)(int wait);
    u32 (*ticks)(void);
    void (*sleep)(u32);
    void* (*alloc)(usize);
    void (*free)(void*);
    int (*find)(const char*);
    int (*read)(int,u32,void*,u32);
    int (*write)(const char*,const void*,u32);
    int (*size)(int);
    void (*rect)(int,int,int,int,u32);
    void (*text)(int,int,const char*,u32);
    void (*present)(void);
    void (*blit)(const u32*,int,int);
    void (*exit)(int);
    int width,height;
    int (*keyevent)(void);
    int (*remove)(const char*);
    int (*mkdir)(const char*);
    int (*rename)(const char*,const char*);
    /* ABI v2 appends services; the v1 prefix remains unchanged. */
    int (*pick_file)(const char*,char*,u32);
    int (*font)(const char*);
    int (*audio_write)(const short*,u32,u32,int);
    int (*music)(const char*,int);
    void (*audio_stop)(void);
    int (*audio_available)(void);
    void (*fullscreen)(int);
    int (*resource)(const char*,void*,u32);
    int (*opened_path)(char*,u32);
    int (*music_data)(const void*,u32,int);
    void (*music_volume)(int);
    /* ABI v3 appends synchronous HTTP and Ethernet status services. */
    int (*net_info)(char*,u32);
    int (*http_get)(const char*,char*,u32);
    /* ABI v4: pointer position in the 800x600 application surface. */
    int (*pointer)(int*,int*);
    /* ABI v5: directory entry (FsEntry), relative input and RTC services. */
    int (*read_dir)(const char*,int,void*); /* 1 entry, 0 end, -1 error */
    int (*mouse_capture)(int);
    int (*mouse_relative)(int*,int*);
    int (*modifiers)(void); /* shift=1, ctrl=2, alt=4 */
    void (*date_time)(int*); /* year, month, day, hour, minute, second */
    /* ABI v6: streaming media and legacy-IDE digital audio extraction. */
    void (*surface_blit)(const u32*,int,int,int,int,int,int);
    void (*viewport)(int*,int*);
    int (*music_control)(int,int); /* 0 state, 1 pause(0/1), 2 elapsed ms */
    int (*cd_count)(void);
    int (*cd_info)(int,CdInfo*);
    int (*cd_read)(int,u32,u32,void*); /* CD-DA sectors, 2352 bytes each */
    void (*audio_pause)(int);
    u32 (*audio_pending)(void); /* milliseconds of queued PCM, excluding hardware tail */
    /* ABI v7: bounded shared clipboard, runtime metrics and direct 4:3 frames. */
    int (*clipboard_set)(const char*,u32); /* max 4095 bytes; copies data */
    int (*clipboard_get)(char*,u32); /* returns bytes excluding NUL, -1 if too small */
    int (*system_info)(BdsSystemInfo*,u32);
    void (*frame_submit)(const u32*,int,int); /* synchronous, does not retain pixels */
} BdsApi;
typedef struct __attribute__((packed)) {
    char magic[4];u16 version,arch;u32 code,bss,entry,memory,checksum,flags;
} RunHeader;
typedef struct __attribute__((packed)) {
    char name[32];u32 parent,type,size,start,capacity;u8 reserved[12];
} FsEntry;
#endif
