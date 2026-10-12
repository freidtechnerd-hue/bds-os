#ifndef BDS_H
#define BDS_H
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned int usize;
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
} BdsApi;
typedef struct __attribute__((packed)) {
    char magic[4];u16 version,arch;u32 code,bss,entry,memory,checksum,flags;
} RunHeader;
typedef struct __attribute__((packed)) {
    char name[32];u32 parent,type,size,start,capacity;u8 reserved[12];
} FsEntry;
#endif
