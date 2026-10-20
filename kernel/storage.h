#ifndef STORAGE_H
#define STORAGE_H
#define MAX_DRIVES 24
#define MAX_VOLUMES 32
typedef struct {u32 blocks;u16 sector_size;u8 bios,readonly;char interface[16];} Drive;
typedef struct {int drive;u32 start,blocks;int type;} Volume;
extern Drive drives[MAX_DRIVES];extern int drive_count,boot_drive;
extern Volume volumes[MAX_VOLUMES];extern int volume_count;
void storage_init(u32);void storage_list(void);
int disk_io(int,u32,u32,void*,int);int volume_io(int,u32,void*,int);
int fs_mount(int);int fs_volume(void);
int media_mount(int);int media_read(int,u32,void*,u32);
int media_visit(int);
int media_write(const char*,const void*,u32);int media_mkdir(const char*);int media_remove(int);int media_rename(int,const char*,u32);
void installer(u32);
#endif
