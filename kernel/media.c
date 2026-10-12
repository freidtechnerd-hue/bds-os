/* FAT12/16/32 (short names) and ISO9660, through the shared block layer.
 * Directory snapshots retain on-disk locations; all FAT copies are updated.
 * No journal: removable media must not be unplugged during a write.
 */
#include "os.h"
#include "storage.h"
typedef struct {u32 cluster,sector;u16 offset;u8 attr,lfn_count;u32 lfn_sector[20];u16 lfn_offset[20];} Location;
static Location loc[128];
static int vol,format,bits;static u32 fatcache_sector=0xffffffff,alloc_cursor=2,info_sector,backup_sector,free_hint=0xffffffff;static u8 fatcache[512];static u32 spc,reserved,fats,fatsz,rootsecs,rootbase,database,rootcluster,clusters,total;
static int io(u32 s,void*b,int w){return volume_io(vol,s,b,w);}
static u32 eoc(void){return bits==12?0xff8:bits==16?0xfff8:0xffffff8;}
static int valid(u32 c){return c>=2&&c<clusters+2;}
static int fatget(u32 c,u32*out){
    if(!valid(c))return -1;u32 off=bits==12?c+c/2:c*(bits/8);u8 b[512],n[512];u32 fs=reserved+off/512;if(fatcache_sector!=fs){if(io(fs,fatcache,0))return -1;fatcache_sector=fs;}memcpy(b,fatcache,512);
    u32 pos=off%512;u32 v=b[pos];for(int j=1;j<(bits==32?4:2);j++){if(pos+j>=512){if(io(reserved+off/512+1,n,0))return -1;v|=(u32)n[pos+j-512]<<(8*j);}else v|=(u32)b[pos+j]<<(8*j);}
    *out=bits==12?(c&1?v>>4:v&0xfff):bits==16?v&0xffff:v&0xfffffff;return 0;
}
static int fatset(u32 c,u32 value){
    if(!valid(c))return -1;u32 old_value=0;if(bits==32&&fatget(c,&old_value))return -1;fatcache_sector=0xffffffff;u32 off=bits==12?c+c/2:c*(bits/8);u8 b[512],n[512];u32 s=off/512,p=off%512;
    for(u32 f=0;f<fats;f++){
        if(io(reserved+f*fatsz+s,b,0))return -1;int cross=bits==12&&p==511;if(cross&&io(reserved+f*fatsz+s+1,n,0))return -1;
        if(bits==12){u16 old=b[p]|((u16)(cross?n[0]:b[p+1])<<8);u16 v=c&1?(old&15)|(value<<4):(old&0xf000)|(value&0xfff);b[p]=v;if(cross)n[0]=v>>8;else b[p+1]=v>>8;}
        else if(bits==16)*(u16*)(b+p)=value;else *(u32*)(b+p)=(*(u32*)(b+p)&0xf0000000)|(value&0xfffffff);
        if(io(reserved+f*fatsz+s,b,1)||(cross&&io(reserved+f*fatsz+s+1,n,1)))return -1;
    }
    if(bits==32&&info_sector&&info_sector<reserved){u8 info[512];if(io(info_sector,info,0))return -1;if(*(u32*)info==0x41615252&&*(u32*)(info+484)==0x61417272){if(free_hint!=0xffffffff){if(!old_value&&value&&free_hint)free_hint--;else if(old_value&&!value&&free_hint<clusters)free_hint++;}*(u32*)(info+488)=free_hint;*(u32*)(info+492)=0xffffffff;if(io(info_sector,info,1))return -1;if(backup_sector&&backup_sector+info_sector<reserved&&io(backup_sector+info_sector,info,1))return -1;}}
    return 0;
}
static int release(u32 c){for(u32 i=0;c&&i<clusters;i++){u32 n;if(!valid(c)||fatget(c,&n)||fatset(c,0))return -1;if(n>=eoc())return 0;c=n;}return c?-1:0;}
static int allocate(u32*out){
    for(u32 step=0;step<clusters;step++){u32 c=alloc_cursor++;if(alloc_cursor>=clusters+2)alloc_cursor=2;u32 v;if(fatget(c,&v))return -1;if(!v){if(fatset(c,0xfffffff))return -1;u8 z[512];memset(z,0,512);for(u32 j=0;j<spc;j++)if(io(database+(c-2)*spc+j,z,1)){fatset(c,0);return -1;}*out=c;return 0;}}return -1;
}
static int freeslot(void){for(int i=0;i<128;i++)if(!files[i].type)return i;return -1;}
static int scan_fat(u32 c,u32 parent,int depth){
    if(depth>16)return -1;u8 b[512];u32 steps=0;u32 lsect[20];u16 loff[20];int lcount=0;
    do{
        u32 base=c?database+(c-2)*spc:rootbase,num=c?spc:rootsecs;if(c&&!valid(c))return -1;
        for(u32 j=0;j<num;j++){if(io(base+j,b,0))return -1;for(int k=0;k<512;k+=32){u8*p=b+k;if(!p[0])goto recurse;if(p[0]==0xe5||p[0]=='.'){lcount=0;continue;}if(p[11]==15){if(lcount<20){lsect[lcount]=base+j;loff[lcount++]=k;}continue;}if(p[11]&8){lcount=0;continue;}
            int i=freeslot();if(i<0)return -1;FsEntry*f=files+i;int n=0;for(int a=0;a<8&&p[a]!=' ';a++)f->name[n++]=p[a];if(p[8]!=' '){f->name[n++]='.';for(int a=8;a<11&&p[a]!=' ';a++)f->name[n++]=p[a];}f->name[n]=0;f->parent=parent;f->type=p[11]&16?2:1;f->size=*(u32*)(p+28);
            u32 cl=*(u16*)(p+26);if(bits==32)cl|=(u32)*(u16*)(p+20)<<16;loc[i].cluster=cl;loc[i].sector=base+j;loc[i].offset=k;loc[i].attr=p[11];loc[i].lfn_count=lcount;for(int a=0;a<lcount;a++){loc[i].lfn_sector[a]=lsect[a];loc[i].lfn_offset[a]=loff[a];}lcount=0;
        }}
        if(!c)break;u32 next;if(fatget(c,&next))return -1;if(next>=eoc())break;c=next;if(++steps>=clusters)return -1;
    }while(1);
recurse:
    /* Recurse after parsing this directory so parent records remain stable. */
    for(int i=0;i<128;i++)if(files[i].type==2&&files[i].parent==parent)if(scan_fat(loc[i].cluster,i+1,depth+1))return -1;
    return 0;
}
static int scan_iso(u32 extent,u32 size,u32 parent,int depth){
    if(depth>16||size>16*1024*1024)return -1;u8 b[2048];
    for(u32 at=0;at<size;){u32 sector=at/2048;if(io(extent+sector,b,0))return -1;u32 p=at%2048;
        while(p<2048&&sector*2048+p<size){u8*r=b+p;u32 len=r[0];if(!len)break;if(len<34||p+len>2048||33u+r[32]>len)return -1;
            if(!(r[32]==1&&r[33]<=1)){int i=freeslot();if(i<0){print("ISO directory snapshot: first 128 entries only.\n");return 0;}FsEntry*f=files+i;int n=0;for(int j=0;j<r[32]&&n<31&&r[33+j]!=';';j++)f->name[n++]=r[33+j];if(n&&f->name[n-1]=='.')n--;f->name[n]=0;f->type=r[25]&2?2:1;f->parent=parent;f->size=*(u32*)(r+10);loc[i].cluster=*(u32*)(r+2);if(r[25]&0x80)return -1;}
            p+=len;
        }at=(sector+1)*2048;
    }
    for(int i=0;i<128;i++)if(files[i].type==2&&files[i].parent==parent)if(scan_iso(loc[i].cluster,files[i].size,i+1,depth+1))return -1;return 0;
}
int media_mount(int id){
    vol=id;format=volumes[id].type;fatcache_sector=0xffffffff;alloc_cursor=2;info_sector=backup_sector=0;free_hint=0xffffffff;memset(loc,0,sizeof(loc));u8 b[2048];
    if(format==3){if(io(16,b,0)||*(u16*)(b+128)!=2048)return -1;return scan_iso(*(u32*)(b+158),*(u32*)(b+166),0,0);}
    if(io(0,b,0))return -1;spc=b[13];reserved=*(u16*)(b+14);fats=b[16];fatsz=*(u16*)(b+22);if(!fatsz)fatsz=*(u32*)(b+36);total=*(u16*)(b+19);if(!total)total=*(u32*)(b+32);
    rootsecs=((u32)*(u16*)(b+17)*32+511)/512;rootbase=reserved+fats*fatsz;database=rootbase+rootsecs;
    if(!spc||spc>128||!fatsz||!total||total>volumes[id].blocks||database>=total)return -1;
    clusters=(total-database)/spc;bits=clusters<4085?12:clusters<65525?16:32;
    if(clusters+2>(bits==12?(fatsz*512*2)/3:(fatsz*512)/(bits/8)))return -1;
    rootcluster=bits==32?*(u32*)(b+44):0;info_sector=bits==32?*(u16*)(b+48):0;backup_sector=bits==32?*(u16*)(b+50):0;
    if(bits==32){/* Mirror both FATs; active-FAT-only volumes are read-only here. */if(*(u16*)(b+40)&0x80)return -1;if(info_sector&&info_sector<reserved){u8 info[512];if(io(info_sector,info,0))return -1;if(*(u32*)info==0x41615252&&*(u32*)(info+484)==0x61417272&&*(u32*)(info+488)<=clusters)free_hint=*(u32*)(info+488);}}
    return scan_fat(rootcluster,0,0);
}
int media_read(int i,u32 off,void*out,u32 count){
    if(i<0||i>=128||files[i].type!=1)return -1;u32 size=files[i].size;if(off>=size)return 0;if(count>size-off)count=size-off;
    u8 b[2048];u32 done=0,c=loc[i].cluster,cs=spc*512;
    if(format==2){for(u32 skip=off/cs;skip;skip--){u32 n;if(!valid(c)||fatget(c,&n))return -1;c=n;}}
    while(done<count){u32 pos=off+done,n,s;
        if(format==3){s=loc[i].cluster+pos/2048;n=2048-pos%2048;}
        else{if(!valid(c))return -1;s=database+(c-2)*spc+(pos%cs)/512;n=512-pos%512;}
        if(n>count-done)n=count-done;if(io(s,b,0))return -1;memcpy((u8*)out+done,b+pos%(format==3?2048:512),n);done+=n;
        if(format==2&&done<count&&(off+done)%cs==0){u32 next;if(fatget(c,&next))return -1;c=next;}
    }return done;
}
static int shortname(const char*name,u8*out){
    memset(out,' ',11);int a=0,b=0,dot=0;for(;*name;name++){char c=*name;if(c=='.'){if(dot||!a)return -1;dot=1;continue;}if(c>='a'&&c<='z')c-=32;if(c<=32||c>=127||c=='/'||c=='\\'||c==':'||c=='*'||c=='?'||c=='"'||c=='<'||c=='>'||c=='|'||c=='+'||c=='='||c==','||c==';'||c=='['||c==']')return -1;if(dot){if(b>=3)return -1;out[8+b++]=c;}else{if(a>=8)return -1;out[a++]=c;}}return a?0:-1;
}
static int resolve(const char*path,char*name,u32*parent){
    int n=strlen(path);if(n<1||n>4095)return -1;char p[4096];strcpy(p,path);int slash=-1;for(int j=0;j<n;j++)if(p[j]=='/'||p[j]=='\\')slash=j;
    if(n-slash-1<1||n-slash-1>31)return -1;strcpy(name,p+slash+1);*parent=cwd;
    if(slash>=0){p[slash]=0;if(!slash)*parent=0;else{int d=fs_find(p);if(d>=0)d%=128;if(d==-2)*parent=0;else if(d<0||files[d].type!=2)return -1;else *parent=d+1;}}
    u8 raw[11];if(shortname(name,raw))return -1;for(int i=0;i<128;i++)if(files[i].type&&files[i].parent==*parent&&!stricmp(files[i].name,name))return i;return -2;
}
static int directory_slot(u32 parent,u32*sector,u16*offset){
    u32 c=parent?loc[parent-1].cluster:rootcluster;u8 b[512];u32 steps=0;
    do{u32 base=c?database+(c-2)*spc:rootbase,num=c?spc:rootsecs;if(c&&!valid(c))return -1;
        for(u32 j=0;j<num;j++){if(io(base+j,b,0))return -1;for(int k=0;k<512;k+=32)if(!b[k]||b[k]==0xe5){*sector=base+j;*offset=k;return 0;}}
        if(!c)return -1;u32 next;if(fatget(c,&next))return -1;if(next>=eoc()){if(allocate(&next)||fatset(c,next))return -1;*sector=database+(next-2)*spc;*offset=0;return 0;}c=next;
    }while(++steps<clusters);return -1;
}
static int entry_update(Location l,const u8*entry){u8 b[512];if(io(l.sector,b,0))return -1;memcpy(b+l.offset,entry,32);return io(l.sector,b,1);}
static void setcluster(u8*p,u32 c){*(u16*)(p+26)=c;*(u16*)(p+20)=bits==32?c>>16:0;}
static int refresh(void){return fs_mount(vol);}
int media_write(const char*path,const void*data,u32 size){
    if(format!=2)return -1;char name[32];u32 parent;int i=resolve(path,name,&parent);if(i==-1||(i>=0&&(files[i].type!=1||(loc[i].attr&1)))||size>32*1024*1024)return -1;
    Location l={0};if(i>=0)l=loc[i];else if(freeslot()<0||directory_slot(parent,&l.sector,&l.offset))return -1;
    u32 first=0,last=0,done=0;u8 b[512];
    while(done<size){u32 c;if(allocate(&c)){release(first);return -1;}if(!first)first=c;if(last&&fatset(last,c)){release(c);release(first);return -1;}last=c;
        for(u32 s=0;s<spc&&done<size;s++){memset(b,0,512);u32 n=size-done;if(n>512)n=512;memcpy(b,(const u8*)data+done,n);if(io(database+(c-2)*spc+s,b,1)){release(first);return -1;}done+=n;}
    }
    u8 entry[32];memset(entry,0,32);shortname(name,entry);entry[11]=32;setcluster(entry,first);*(u32*)(entry+28)=size;
    if(entry_update(l,entry)){release(first);return -1;}if(i>=0&&release(l.cluster))return -1;return refresh();
}
int media_mkdir(const char*path){
    if(format!=2)return -1;char name[32];u32 parent;if(resolve(path,name,&parent)!=-2||freeslot()<0)return -1;
    Location l={0};u32 c;if(directory_slot(parent,&l.sector,&l.offset)||allocate(&c))return -1;
    u8 b[512];memset(b,0,512);memset(b,' ',11);b[0]='.';b[11]=16;setcluster(b,c);memset(b+32,' ',11);b[32]=b[33]='.';b[43]=16;setcluster(b+32,parent?loc[parent-1].cluster:0);
    if(io(database+(c-2)*spc,b,1)){release(c);return -1;}u8 e[32];memset(e,0,32);shortname(name,e);e[11]=16;setcluster(e,c);if(entry_update(l,e)){release(c);return -1;}return refresh();
}
static int clear_lfn(Location*l){u8 b[512];for(int j=0;j<l->lfn_count;j++){if(io(l->lfn_sector[j],b,0))return -1;b[l->lfn_offset[j]]=0xe5;if(io(l->lfn_sector[j],b,1))return -1;}return 0;}
int media_remove(int i){
    if(format!=2||(loc[i].attr&1))return -1;u8 b[512];Location l=loc[i];if(io(l.sector,b,0))return -1;b[l.offset]=0xe5;if(io(l.sector,b,1)||clear_lfn(&l)||release(l.cluster))return -1;return refresh();
}
int media_rename(int i,const char*name,u32 parent){
    /* Moving a directory requires updating its '..' entry; supported below. */
    if(format!=2||(loc[i].attr&1))return -1;u8 raw[11];if(shortname(name,raw))return -1;Location old=loc[i],dest=old;u8 b[512],entry[32];
    if(io(old.sector,b,0))return -1;memcpy(entry,b+old.offset,32);memcpy(entry,raw,11);
    if(parent!=files[i].parent){if(directory_slot(parent,&dest.sector,&dest.offset))return -1;if(files[i].type==2){u32 s=database+(old.cluster-2)*spc;u8 dot[512];if(io(s,dot,0))return -1;setcluster(dot+32,parent?loc[parent-1].cluster:0);if(io(s,dot,1))return -1;}}
    if(entry_update(dest,entry)||clear_lfn(&old))return -1;if(dest.sector!=old.sector||dest.offset!=old.offset){if(io(old.sector,b,0))return -1;b[old.offset]=0xe5;if(io(old.sector,b,1))return -1;}return refresh();
}
