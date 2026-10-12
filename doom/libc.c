#include "libc.h"
#include "bds.h"
extern BdsApi *bds;
#ifndef BDS_UTILITY
extern void doom_loading(const char*,u32,u32);
#endif
int errno;static FILE out_file;FILE *stdout=&out_file,*stderr=&out_file,*stdin=&out_file;
void *memcpy(void*d,const void*s,size_t n){unsigned char*a=d;const unsigned char*b=s;while(n--)*a++=*b++;return d;}
void *memmove(void*d,const void*s,size_t n){unsigned char*a=d;const unsigned char*b=s;if(a<b)return memcpy(d,s,n);while(n){n--;a[n]=b[n];}return d;}
void *memset(void*d,int c,size_t n){unsigned char*a=d;while(n--)*a++=c;return d;}
int memcmp(const void*a,const void*b,size_t n){const unsigned char*x=a,*y=b;while(n--){if(*x!=*y)return *x-*y;x++;y++;}return 0;}
void *malloc(size_t n){if(n>0x3fffff0)return NULL;size_t*p=bds->alloc(n+16);if(!p)return NULL;*p=n;return (char*)p+16;}
void free(void*p){if(p)bds->free((char*)p-16);}
void *calloc(size_t n,size_t s){if(s&&n>SIZE_MAX/s)return NULL;void*p=malloc(n*s);if(p)memset(p,0,n*s);return p;}
void *realloc(void*p,size_t n){if(!p)return malloc(n);if(!n){free(p);return NULL;}size_t old=*(size_t*)((char*)p-16);void*q=malloc(n);if(!q)return NULL;memcpy(q,p,n<old?n:old);free(p);return q;}
size_t strlen(const char*s){size_t n=0;while(s[n])n++;return n;}
char *strcpy(char*d,const char*s){char*r=d;while((*d++=*s++));return r;}
char *strncpy(char*d,const char*s,size_t n){char*r=d;while(n--){*d++=*s;if(*s)s++;}return r;}
char *strcat(char*d,const char*s){strcpy(d+strlen(d),s);return d;}
int strcmp(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return (unsigned char)*a-(unsigned char)*b;}
int strncmp(const char*a,const char*b,size_t n){while(n--){if(*a!=*b)return (unsigned char)*a-(unsigned char)*b;if(!*a)return 0;a++;b++;}return 0;}
int tolower(int c){return c>='A'&&c<='Z'?c+32:c;}int toupper(int c){return c>='a'&&c<='z'?c-32:c;}
int strncasecmp(const char*a,const char*b,size_t n){while(n--){int x=tolower((unsigned char)*a++),y=tolower((unsigned char)*b++);if(x!=y)return x-y;if(!x)return 0;}return 0;}
int strcasecmp(const char*a,const char*b){return strncasecmp(a,b,SIZE_MAX);}
char *strchr(const char*s,int c){do{if(*s==c)return (char*)s;}while(*s++);return NULL;}
char *strrchr(const char*s,int c){char*p=NULL;do{if(*s==c)p=(char*)s;}while(*s++);return p;}
char *strstr(const char*s,const char*n){size_t z=strlen(n);do{if(!strncmp(s,n,z))return (char*)s;}while(*s++);return NULL;}
char *strdup(const char*s){char*p=malloc(strlen(s)+1);if(p)strcpy(p,s);return p;}
int isspace(int c){return c==' '||(c>=9&&c<=13);}int isprint(int c){return c>=32&&c<127;}int isdigit(int c){return c>='0'&&c<='9';}int isalpha(int c){c=tolower(c);return c>='a'&&c<='z';}
int isalnum(int c){return isalpha(c)||isdigit(c);}
long strtol(const char*s,char**end,int base){while(isspace(*s))s++;int sign=1;unsigned v=0;if(*s=='-'){sign=-1;s++;}else if(*s=='+')s++;if(!base){base=10;if(*s=='0')base=8;if(s[0]=='0'&&tolower(s[1])=='x'){base=16;s+=2;}}else if(base==16&&s[0]=='0'&&tolower(s[1])=='x')s+=2;while(*s){int d=isdigit(*s)?*s-'0':tolower(*s)-'a'+10;if(d<0||d>=base)break;v=v*base+d;s++;}if(end)*end=(char*)s;return sign<0?0u-v:v;}
int atoi(const char*s){return strtol(s,NULL,10);}int abs(int n){return n<0?-n:n;}
double atof(const char*s){while(isspace(*s))s++;int sign=1;if(*s=='-'){sign=-1;s++;}double v=0;while(isdigit(*s))v=v*10+*s++-'0';if(*s=='.'){double p=.1;s++;while(isdigit(*s)){v+=(*s++-'0')*p;p*=.1;}}return v*sign;}
char *getenv(const char*s){return NULL;}int system(const char*s){errno=38;return -1;}
void exit(int status){
#ifndef BDS_UTILITY
    bds->print("DOOM exit to BDS-OS\n");
#else
    bds->print("UTILITY exit to BDS-OS\n");
#endif
    bds->exit(status);for(;;);
}
void abort(void){exit(1);}void bds_assert(const char*file,int line){printf("Assertion failed at %s:%d\n",file,line);exit(1);}
FILE *fopen(const char*path,const char*mode){if(strlen(path)>255){errno=36;return NULL;}int id=bds->find(path),size=bds->size(id),write=*mode=='w'||*mode=='a';if(size<0&&!write){errno=id>=0||id==-2?EISDIR:ENOENT;return NULL;}FILE*f=calloc(1,sizeof(FILE));if(!f)return NULL;f->id=id;f->mode=write;strcpy(f->path,path);f->size=size<0||*mode=='w'?0:size;f->capacity=f->size?f->size:4096;f->data=malloc(f->capacity);if(!f->data){free(f);return NULL;}
    for(u32 off=0;off<f->size;){u32 n=f->size-off;if(n>65536)n=65536;
#ifndef BDS_UTILITY
        if(f->size>1024*1024&&!(off%4194304))doom_loading("Reading WAD",off,f->size);
#endif
        if(bds->read(id,off,f->data+off,n)!=(int)n){free(f->data);free(f);return NULL;}off+=n;}
#ifndef BDS_UTILITY
    if(f->size>1024*1024)doom_loading("Preparing textures, sound and game engine",0,0);
#endif
    if(*mode=='a')f->pos=f->size;return f;}
int fflush(FILE*f){if(f==stdout||f==stderr)return 0;if(f&&f->mode&&bds->write(f->path,f->data,f->size)){f->error=1;return EOF;}return 0;}
int fclose(FILE*f){if(!f)return EOF;if(f==stdout||f==stderr)return 0;int r=fflush(f);free(f->data);free(f);return r;}
size_t fread(void*out,size_t size,size_t n,FILE*f){if(!size||n>SIZE_MAX/size)return 0;size_t bytes=size*n;if(f->pos>=f->size)return 0;if(bytes>f->size-f->pos)bytes=f->size-f->pos;memcpy(out,f->data+f->pos,bytes);f->pos+=bytes;return bytes/size;}
size_t fwrite(const void*in,size_t size,size_t n,FILE*f){if(!size||n>SIZE_MAX/size)return 0;size_t bytes=size*n;if(f==stdout||f==stderr){const char*s=in;for(size_t i=0;i<bytes;i++)putchar(s[i]);return n;}if(!f->mode||bytes>32*1024*1024-f->pos){f->error=1;return 0;}size_t end=f->pos+bytes;if(end>f->capacity){size_t cap=f->capacity;while(cap<end)cap*=2;void*p=realloc(f->data,cap);if(!p){f->error=1;return 0;}f->data=p;f->capacity=cap;}if(f->pos>f->size)memset(f->data+f->size,0,f->pos-f->size);memcpy(f->data+f->pos,in,bytes);f->pos=end;if(end>f->size)f->size=end;return n;}
int fseek(FILE*f,long off,int whence){long pos=off;if(whence==SEEK_CUR)pos+=f->pos;else if(whence==SEEK_END)pos+=f->size;if(pos<0)return -1;f->pos=pos;return 0;}long ftell(FILE*f){return f->pos;}int feof(FILE*f){return f->pos>=f->size;}int ferror(FILE*f){return f->error;}
int fgetc(FILE*f){unsigned char c;return fread(&c,1,1,f)==1?c:EOF;}int fputc(int c,FILE*f){unsigned char b=c;return fwrite(&b,1,1,f)==1?b:EOF;}
char *fgets(char*s,int n,FILE*f){int i=0,c;while(i<n-1&&(c=fgetc(f))!=EOF){s[i++]=c;if(c=='\n')break;}s[i]=0;return i?s:NULL;}
int remove(const char*p){return bds->remove(p);}int rename(const char*a,const char*b){return bds->rename(a,b);}int mkdir(const char*p,int mode){char s[256];if(strlen(p)>255)return -1;strcpy(s,p);size_t n=strlen(s);while(n>1&&s[n-1]=='/')s[--n]=0;return bds->mkdir(s);}
static void emit(char*d,size_t max,size_t*pos,char c){if(*pos+1<max)d[*pos]=c;(*pos)++;}
int vsnprintf(char*d,size_t max,const char*fmt,va_list ap){size_t pos=0;while(*fmt){if(*fmt!='%'){emit(d,max,&pos,*fmt++);continue;}fmt++;int left=0,zero=0,width=0,prec=-1;if(*fmt=='-'){left=1;fmt++;}if(*fmt=='0'){zero=1;fmt++;}while(isdigit(*fmt))width=width*10+*fmt++-'0';if(*fmt=='.'){fmt++;prec=0;while(isdigit(*fmt))prec=prec*10+*fmt++-'0';}while(*fmt=='l'||*fmt=='h'||*fmt=='z')fmt++;char spec=*fmt++;char buf[96];const char*s=buf;int n=0;if(spec=='s'){s=va_arg(ap,const char*);if(!s)s="(null)";n=strlen(s);if(prec>=0&&prec<n)n=prec;}else if(spec=='c'){buf[0]=va_arg(ap,int);n=1;}else if(spec=='%'){buf[0]='%';n=1;}else if(spec=='d'||spec=='i'||spec=='u'||spec=='x'||spec=='X'||spec=='p'){unsigned v=va_arg(ap,unsigned),base=(spec=='x'||spec=='X'||spec=='p')?16:10;int neg=(spec=='d'||spec=='i')&&(int)v<0;char rev[40];if(neg)v=0u-v;int r=0;do{unsigned t=v%base;rev[r++]=t<10?'0'+t:(spec=='X'?'A':'a')+t-10;v/=base;}while(v);if(neg)buf[n++]='-';while(r)buf[n++]=rev[--r];}else if(spec=='f'){double v=va_arg(ap,double);if(v<0){buf[n++]='-';v=-v;}unsigned whole=v;char rev[24];int r=0;do{rev[r++]='0'+whole%10;whole/=10;}while(whole);while(r)buf[n++]=rev[--r];buf[n++]='.';v-=(unsigned)v;int p=prec<0?6:prec;if(p>12)p=12;while(p--){v*=10;int t=v;buf[n++]='0'+t;v-=t;}}else{buf[n++]=spec;}
    if(prec>n&&(spec=='d'||spec=='i'||spec=='u'||spec=='x'||spec=='X')){int extra=prec-n;if(extra>80-n)extra=80-n;int sign=n&&buf[0]=='-';memmove(buf+sign+extra,buf+sign,n-sign);memset(buf+sign,'0',extra);n+=extra;}
    int pad=width-n;if(pad>1024)pad=1024;if(!left)while(pad-->0)emit(d,max,&pos,zero?'0':' ');for(int i=0;i<n;i++)emit(d,max,&pos,s[i]);if(left)while(pad-->0)emit(d,max,&pos,' ');}
    if(max)d[pos<max?pos:max-1]=0;return pos;}
int snprintf(char*d,size_t n,const char*f,...){va_list ap;va_start(ap,f);int r=vsnprintf(d,n,f,ap);va_end(ap);return r;}
int sprintf(char*d,const char*f,...){va_list ap;va_start(ap,f);int r=vsnprintf(d,SIZE_MAX,f,ap);va_end(ap);return r;}
int vfprintf(FILE*f,const char*fmt,va_list ap){char buf[2048];int n=vsnprintf(buf,sizeof(buf),fmt,ap);fwrite(buf,1,strlen(buf),f);return n;}
int printf(const char*fmt,...){va_list ap;va_start(ap,fmt);int r=vfprintf(stdout,fmt,ap);va_end(ap);return r;}
int fprintf(FILE*f,const char*fmt,...){va_list ap;va_start(ap,fmt);int r=vfprintf(f,fmt,ap);va_end(ap);return r;}
int putchar(int c){char s[2]={c,0};bds->print(s);return c;}int puts(const char*s){bds->print(s);bds->print("\n");return 0;}
static int vscan(const char*s,const char*f,va_list ap){int done=0;while(*f){if(isspace(*f)){while(isspace(*s))s++;f++;continue;}if(*f!='%'){if(*s++!=*f++)return done;continue;}f++;int width=0;while(isdigit(*f))width=width*10+*f++-'0';if(!width)width=1024;char spec=*f++;if(spec=='d'||spec=='i'||spec=='x'||spec=='o'){while(isspace(*s))s++;char*end;long v=strtol(s,&end,spec=='x'?16:spec=='o'?8:spec=='i'?0:10);if(end==s)return done;*va_arg(ap,int*)=v;s=end;done++;}else if(spec=='s'){while(isspace(*s))s++;char*d=va_arg(ap,char*);int n=0;while(*s&&!isspace(*s)&&n<width)d[n++]=*s++;d[n]=0;if(!n)return done;done++;}else if(spec=='['){int invert=*f=='^';if(invert)f++;char stop=*f;while(*f&&*f!=']')f++;if(*f)f++;char*d=va_arg(ap,char*);int n=0;while(*s&&n<width&&(invert?*s!=stop:*s==stop))d[n++]=*s++;d[n]=0;if(!n)return done;done++;}else return done;}return done;}
int sscanf(const char*s,const char*f,...){va_list ap;va_start(ap,f);int r=vscan(s,f,ap);va_end(ap);return r;}
int fscanf(FILE*f,const char*fmt,...){char line[512];if(!fgets(line,sizeof(line),f))return EOF;va_list ap;va_start(ap,fmt);int r=vscan(line,fmt,ap);va_end(ap);return r;}
double fabs(double x){return x<0?-x:x;}double sin(double x){__asm__ volatile("fsin":"+t"(x));return x;}double atan(double x){double out;__asm__ volatile("fld1; fpatan":"=t"(out):"0"(x));return out;}
uint64_t __udivdi3(uint64_t n,uint64_t d){if(!d)abort();uint64_t q=0,r=0;for(int i=63;i>=0;i--){uint64_t high=r>>63;r=(r<<1)|((n>>i)&1);if(high||r>=d){r-=d;q|=(uint64_t)1<<i;}}return q;}
int64_t __divdi3(int64_t n,int64_t d){uint64_t a=n<0?0ull-(uint64_t)n:(uint64_t)n,b=d<0?0ull-(uint64_t)d:(uint64_t)d,q=__udivdi3(a,b);return (n<0)!=(d<0)?0ull-q:q;}
