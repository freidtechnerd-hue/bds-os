/* Read-only BDS filesystem adapter for the upstream DVD IFO and VM engines.
   No host device access, CSS decryption, file writes or background threads. */
#include "libc.h"
#include "bds.h"
#include "dvdread_internal.h"
#include <sys/time.h>
extern BdsApi*bds;
struct dvd_reader_device_s {char root[240];};
struct dvd_file_s {int handles[9];u32 sizes[9];int parts;uint64_t size,pos;};
static unsigned random_state=1;
void srand(unsigned s){random_state=s?s:1;}
int rand(void){random_state=random_state*1664525u+1013904223u;return random_state>>1;}
int gettimeofday(struct timeval*t,void*z){u32 now=bds->ticks();t->tv_sec=now/1000;t->tv_usec=(now%1000)*1000;return 0;}
/* Upstream's optional raw-volume label probe is unavailable on virtual paths. */
int open(const char*p,int flags,...){return -1;}
int close(int f){return -1;}
ssize_t read(int f,void*p,size_t n){return -1;}
off_t lseek(int f,off_t p,int w){return -1;}
dvd_reader_t*DVDOpen2(void*priv,const dvd_logger_cb*log,const char*path){
    if(!path||strlen(path)>230)return NULL;char name[256];snprintf(name,sizeof(name),"%s/VIDEO_TS.IFO",path);if(bds->size(bds->find(name))<2048)return NULL;
    dvd_reader_t*d=calloc(1,sizeof(*d));if(!d)return NULL;d->rd=calloc(1,sizeof(struct dvd_reader_device_s));if(!d->rd){free(d);return NULL;}
    strcpy(((struct dvd_reader_device_s*)d->rd)->root,path);d->priv=priv;if(log)d->logcb=*log;return d;
}
dvd_reader_t*DVDOpen(const char*p){return DVDOpen2(NULL,NULL,p);}
dvd_reader_t*DVDOpenStream2(void*p,const dvd_logger_cb*l,dvd_reader_stream_cb*c){return NULL;}
void DVDClose(dvd_reader_t*d){if(d){free(d->rd);free(d);}}
dvd_file_t*DVDOpenFile(dvd_reader_t*d,int title,dvd_read_domain_t domain){
    if(!d||title<0||title>99||domain>DVD_READ_TITLE_VOBS)return NULL;
    dvd_file_t*f=calloc(1,sizeof(*f));if(!f)return NULL;
    for(int i=0;i<(domain==DVD_READ_TITLE_VOBS?9:1);i++){
        char name[256],base[24];if(title)snprintf(base,sizeof(base),"VTS_%02d_%d",title,domain==DVD_READ_TITLE_VOBS?i+1:0);else strcpy(base,"VIDEO_TS");
        snprintf(name,sizeof(name),"%s/%s.%s",((struct dvd_reader_device_s*)d->rd)->root,base,domain==DVD_READ_INFO_FILE?"IFO":domain==DVD_READ_INFO_BACKUP_FILE?"BUP":"VOB");
        int h=bds->find(name),n=bds->size(h);if(n<=0)break;f->handles[f->parts]=h;f->sizes[f->parts++]=n;f->size+=n;
    }if(!f->parts){free(f);return NULL;}return f;
}
void DVDCloseFile(dvd_file_t*f){free(f);}
static int read_at(dvd_file_t*f,uint64_t at,void*buf,unsigned n){
    if(!f||at>f->size||n>f->size-at)return -1;unsigned done=0;
    for(int i=0;i<f->parts&&done<n;i++){if(at>=f->sizes[i]){at-=f->sizes[i];continue;}unsigned take=f->sizes[i]-(u32)at;if(take>n-done)take=n-done;
        if(bds->read(f->handles[i],(u32)at,(u8*)buf+done,take)!=(int)take)return -1;done+=take;at=0;
    }return done;
}
ssize_t DVDReadBlocks(dvd_file_t*f,int offset,size_t n,unsigned char*data){if(offset<0||n>32768)return -1;int r=read_at(f,(uint64_t)offset*2048,data,n*2048);return r<0?-1:r/2048;}
int32_t DVDFileSeek(dvd_file_t*f,int32_t at){if(!f||at<0||(uint64_t)at>f->size)return -1;f->pos=at;return at;}
int DVDFileSeekForce(dvd_file_t*f,int at,int force){return DVDFileSeek(f,at);}
ssize_t DVDReadBytes(dvd_file_t*f,void*p,size_t n){if(!f||n>INT_MAX)return 0;int r=read_at(f,f->pos,p,n);if(r<0)return 0;f->pos+=r;return r;}
ssize_t DVDFileSize(dvd_file_t*f){return f?(ssize_t)(f->size/2048):0;}
int DVDUDFVolumeInfo(dvd_reader_t*d,char*v,unsigned n,unsigned char*s,unsigned m){return -1;}
int DVDISOVolumeInfo(dvd_reader_t*d,char*v,unsigned n,unsigned char*s,unsigned m){if(n)v[0]=0;if(m)s[0]=0;return 0;}
int DVDDiscID(dvd_reader_t*d,unsigned char*p){return -1;}
