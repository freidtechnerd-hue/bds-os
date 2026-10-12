#include "os.h"
#include "storage.h"
#define fs_find local_find
#define fs_read local_read
#define fs_size local_size
#define fs_write local_write
#define fs_mkdir local_mkdir
#define fs_remove local_remove
#define fs_rename local_rename
FsEntry files[128];int cwd;
static u32 disk_blocks;static int mounted;
static int active=-1,kind;int fs_system_volume=-1;
static int sector(u32 lba,void*buf,int write){return volume_io(active,lba,buf,write);}
static int syncfs(void){for(int i=0;i<16;i++)if(sector(i+1,(u8*)files+i*512,1))return -1;return 0;}
int fs_volume(void){return active;}
int fs_check(void){
    if(!mounted)return -1;if(kind!=1)return 0;
    for(int i=0;i<128;i++){FsEntry*f=files+i;if(!f->type)continue;
        int terminated=0;for(int c=0;c<32;c++)if(!f->name[c])terminated=1;
        if(f->type>2||!f->name[0]||!terminated||f->parent>128)return -1;
        if(f->parent&&files[f->parent-1].type!=2)return -1;
        u32 parent=f->parent;for(int depth=0;parent;depth++){if(depth>=128||parent==(u32)i+1)return -1;parent=files[parent-1].parent;}
        if(f->type==1&&(f->start<17||f->start>=disk_blocks||f->capacity>disk_blocks-f->start||f->size>f->capacity*512))return -1;
        for(int j=0;j<i;j++){FsEntry*g=files+j;if(!g->type)continue;if(f->parent==g->parent&&!stricmp(f->name,g->name))return -1;
            if(f->type==1&&g->type==1&&f->start<g->start+g->capacity&&g->start<f->start+f->capacity)return -1;}
    }
    return 0;
}
int fs_mount(int id){
    if(id<0||id>=volume_count)return -1;
    FsEntry old[128];memcpy(old,files,sizeof(files));int oa=active,ok=kind,om=mounted,oc=cwd;u32 ob=disk_blocks;
    active=id;kind=volumes[id].type;cwd=0;mounted=0;memset(files,0,sizeof(files));
    int r=-1;u8 buf[512];
    if(kind==1&&!sector(0,buf,0)&&!memcmp(buf,"BDSFS01",8)){
        disk_blocks=*(u32*)(buf+8);if(disk_blocks>=1024&&disk_blocks<=0x1000000&&disk_blocks<=volumes[id].blocks&&*(u32*)(buf+12)==128){
            r=0;for(int i=0;i<16;i++)if(sector(i+1,(u8*)files+i*512,0))r=-1;mounted=1;if(fs_check())r=-1;
        }
    }else if(kind!=1)r=media_mount(id);
    if(r){memcpy(files,old,sizeof(files));active=oa;kind=ok;mounted=om;cwd=oc;disk_blocks=ob;if(om&&ok!=1){memset(files,0,sizeof(files));media_mount(oa);cwd=oc;}return -1;}
    mounted=1;return 0;
}
int fs_init(void){
    /* Prefer the boot disk's BDSFS partition; then any validated BDSFS. */
    for(int pass=0;pass<2;pass++)for(int i=0;i<volume_count;i++)if(volumes[i].type==1&&(pass||drives[volumes[i].drive].bios==boot_drive))if(!fs_mount(i)){fs_system_volume=i;return 0;}
    for(int i=0;i<volume_count;i++)if(!fs_mount(i)){fs_system_volume=i;return 0;}return -1;
}
static int child(u32 parent,const char*name){for(int i=0;i<128;i++)if(files[i].type&&files[i].parent==parent&&!stricmp(files[i].name,name))return i;return -1;}
static int walk(const char*path,int want_parent,char*leaf){
    if(!mounted||!path||!*path)return -1;u32 parent=cwd;if(*path=='/'||*path=='\\'){parent=0;path++;}
    while(*path){while(*path=='/'||*path=='\\')path++;if(!*path)break;char part[32];int n=0;while(*path&&*path!='/'&&*path!='\\'){if(n>=31)return -1;part[n++]=*path++;}part[n]=0;
        if(!*path&&want_parent){strcpy(leaf,part);return parent;}
        if(!strcmp(part,"."))continue;if(!strcmp(part,"..")){if(parent)parent=files[parent-1].parent;continue;}
        int i=child(parent,part);if(i<0)return -1;if(!*path)return i;if(files[i].type!=2)return -1;parent=i+1;
    }
    return want_parent?-1:(parent?(int)parent-1:-2);
}
int fs_find(const char*path){return walk(path,0,0);}
int fs_cd(const char*path){int i=fs_find(path);if(i==-2){cwd=0;return 0;}if(i<0||files[i].type!=2)return -1;cwd=i+1;return 0;}
void fs_pwd(char*out){int ids[128],n=0;u32 p=cwd;while(p&&n<128){ids[n++]=p;p=files[p-1].parent;}*out++='/';while(n){const char*s=files[ids[--n]-1].name;while(*s)*out++=*s++;if(n)*out++='/';}*out=0;}
int fs_size(int i){return mounted&&i>=0&&i<128&&files[i].type==1?(int)files[i].size:-1;}
int fs_read(int i,u32 off,void*out,u32 count){
    if(kind!=1)return media_read(i,off,out,count);int len=fs_size(i);if(len<0)return -1;if(off>=(u32)len)return 0;if(count>(u32)len-off)count=len-off;
    u32 done=0;u8 buf[512];Volume*v=volumes+active;
    while(done<count){u32 pos=off+done,left=count-done;
        if(!(pos%512)&&left>=512){u32 blocks=left/512;if(blocks>64)blocks=64;u32 lba=files[i].start+pos/512;if(lba>=v->blocks||blocks>v->blocks-lba||disk_io(v->drive,v->start+lba,blocks,(u8*)out+done,0))return -1;done+=blocks*512;audio_pump();}
        else{u32 n=512-pos%512;if(n>left)n=left;if(sector(files[i].start+pos/512,buf,0))return -1;memcpy((u8*)out+done,buf+pos%512,n);done+=n;}
    }return done;
}
static int destination(const char*path,char*name,u32*parent){int p=walk(path,1,name);if(p<0||!strcmp(name,".")||!strcmp(name,".."))return -3;*parent=p;return child(p,name);}
static int slot(void){for(int i=0;i<128;i++)if(!files[i].type)return i;return -1;}
int fs_write(const char*path,const void*data,u32 size){
    if(!mounted||size>32*1024*1024)return -1;if(kind!=1)return media_write(path,data,size);char name[32];u32 parent;int i=destination(path,name,&parent);if(i==-3)return -1;if(i<0){i=slot();if(i<0)return -1;}else if(files[i].type!=1)return -1;
    u32 blocks=(size+511)/512;if(blocks<8)blocks=8;u32 start=17;
    for(;;){if(start>disk_blocks||blocks>disk_blocks-start)return -1;int overlap=0;for(int j=0;j<128;j++)if(files[j].type==1&&start<files[j].start+files[j].capacity&&files[j].start<start+blocks){start=files[j].start+files[j].capacity;overlap=1;break;}if(!overlap)break;}
    u8 buf[512];for(u32 b=0;b<blocks;b++){memset(buf,0,512);u32 pos=b*512,n=pos<size?size-pos:0;if(n>512)n=512;if(n)memcpy(buf,(const u8*)data+pos,n);if(sector(start+b,buf,1))return -1;}
    FsEntry old=files[i];memset(files+i,0,sizeof(FsEntry));strcpy(files[i].name,name);files[i].parent=parent;files[i].type=1;files[i].size=size;files[i].start=start;files[i].capacity=blocks;
    if(syncfs()){files[i]=old;return -1;}return 0;
}
int fs_mkdir(const char*path){if(kind!=1)return media_mkdir(path);char name[32];u32 parent;int d=destination(path,name,&parent);if(d!=-1){serial("MKDIR: exists or invalid destination\n");return -1;}int i=slot();if(i<0)return -1;memset(files+i,0,sizeof(FsEntry));strcpy(files[i].name,name);files[i].parent=parent;files[i].type=2;int r=syncfs();if(r)serial("MKDIR: disk sync failed\n");return r;}
int fs_remove(const char*path){int i=fs_find(path);if(i<0)return -1;for(int j=0;j<128;j++)if(files[j].type&&files[j].parent==(u32)i+1)return -1;if(cwd==i+1)return -1;if(kind!=1)return media_remove(i);FsEntry old=files[i];memset(files+i,0,sizeof(FsEntry));if(syncfs()){files[i]=old;return -1;}return 0;}
int fs_rename(const char*src,const char*dst){int i=fs_find(src);char name[32];u32 parent;if(i<0||destination(dst,name,&parent)!=-1)return -1;u32 p=parent;while(p){if(p==(u32)i+1)return -1;p=files[p-1].parent;}if(kind!=1)return media_rename(i,name,parent);FsEntry old=files[i];files[i].parent=parent;strcpy(files[i].name,name);if(syncfs()){files[i]=old;return -1;}return 0;}
void fs_list(void){for(int i=0;i<128;i++)if(files[i].type&&files[i].parent==(u32)cwd){print(files[i].name);if(files[i].type==2)print("/\n");else{char n[16];number(n,files[i].size);print("  ");print(n);print(" bytes\n");}}}

#undef fs_find
#undef fs_read
#undef fs_size
#undef fs_write
#undef fs_mkdir
#undef fs_remove
#undef fs_rename

/* Vn:/ paths and opaque handles retain the selected volume across dialogs.
   Temporarily visiting a volume never changes the caller's browsing context. */
static int path_volume(const char**p){
    const char*s=*p;if((s[0]=='V'||s[0]=='v')&&s[1]>='0'&&s[1]<='9'){
        int v=0;s++;while(*s>='0'&&*s<='9'){v=v*10+*s++-'0';if(v>=MAX_VOLUMES)return -1;}
        if(*s!=':'||(s[1]!='/'&&s[1]!='\\'))return -1;*p=s+1;return v;
    }return active;
}
int fs_qualify(int volume,const char*path,char*out,u32 cap){
    if(!path||!out||volume<0||volume>=volume_count)return -1;
    if((path[0]=='V'||path[0]=='v')&&path[1]>='0'&&path[1]<='9'){
        const char*p=path;if(path_volume(&p)<0||strlen(path)+1>cap)return -1;strcpy(out,path);return 0;
    }
    char prefix[16],dir[4096];prefix[0]='V';number(prefix+1,volume);strcpy(prefix+strlen(prefix),":");
    dir[0]=0;if(path[0]!='/'&&path[0]!='\\'){if(volume==active)fs_pwd(dir);else strcpy(dir,"/");if(strcmp(dir,"/"))strcpy(dir+strlen(dir),"/");}
    if(strlen(prefix)+strlen(dir)+strlen(path)+1>cap)return -1;
    strcpy(out,prefix);strcpy(out+strlen(out),dir);strcpy(out+strlen(out),path);return 0;
}
static int visit(int v){return v<0||v>=volume_count?-1:v==active?0:fs_mount(v);}
static void restore(int v,int c){if(active!=v&&v>=0)fs_mount(v);cwd=c;}
int fs_find(const char*path){if(!path)return -1;int old=active,c=cwd,v=path_volume(&path);if(visit(v))return -1;int f=local_find(path);restore(old,c);return f<0?f:(v+1)*128+f;}
int fs_type(int f){int old=active,c=cwd,v=f>=128?f/128-1:active;if(f<0||visit(v))return -1;int t=files[f%128].type;restore(old,c);return t;}
int fs_size(int f){int old=active,c=cwd,v=f>=128?f/128-1:active;if(f<0||visit(v))return -1;int n=local_size(f%128);restore(old,c);return n;}
int fs_read(int f,u32 off,void*out,u32 n){int old=active,c=cwd,v=f>=128?f/128-1:active;if(f<0||visit(v))return -1;int r=local_read(f%128,off,out,n);restore(old,c);return r;}
int fs_write(const char*path,const void*data,u32 n){int old=active,c=cwd,v=path_volume(&path);if(visit(v))return -1;int r=local_write(path,data,n);restore(old,c);return r;}
int fs_mkdir(const char*path){int old=active,c=cwd,v=path_volume(&path);if(visit(v))return -1;int r=local_mkdir(path);restore(old,c);return r;}
int fs_remove(const char*path){int old=active,c=cwd,v=path_volume(&path);if(visit(v))return -1;int r=local_remove(path);restore(old,c);return r;}
int fs_rename(const char*src,const char*dst){int old=active,c=cwd,v=path_volume(&src),w=path_volume(&dst);if(v!=w||visit(v))return -1;int r=local_rename(src,dst);restore(old,c);return r;}
int fs_next_volume(int step){int start=active;for(int i=1;i<=volume_count;i++){int v=(start+i*step+volume_count*2)%volume_count;if(!fs_mount(v)){char n[16];number(n,v);serial("FILE MANAGER VOLUME: ");serial(n);serial("\n");return 0;}}return -1;}
void fs_volume_label(char*out){char n[16];strcpy(out,"Drive V");number(n,active);strcpy(out+strlen(out),n);if(active>=0){Drive*d=drives+volumes[active].drive;strcpy(out+strlen(out),"  ");strcpy(out+strlen(out),d->interface);strcpy(out+strlen(out),volumes[active].type==1?" BDSFS":volumes[active].type==2?" FAT":" CD/DVD");}}
int fs_copy(const char*src,const char*dst){int i=fs_find(src),n=fs_size(i);if(n<0)return -1;void*b=alloc(n?n:1);if(!b)return -1;int r=fs_read(i,0,b,n)==n?fs_write(dst,b,n):-1;dealloc(b);return r;}
