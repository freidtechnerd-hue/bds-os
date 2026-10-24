/* Small freestanding string/sort support for FreeType; no host file access. */
#include <stddef.h>
#include <limits.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SYSTEM_H
#include FT_ERRORS_H
void*memchr(const void*p,int c,size_t n){const unsigned char*s=p;while(n--){if(*s==(unsigned char)c)return(void*)s;s++;}return 0;}
char*strcat(char*d,const char*s){char*r=d;while(*d)d++;while((*d++=*s++));return r;}
int strncmp(const char*a,const char*b,size_t n){while(n&&*a&&*a==*b){a++;b++;n--;}return n?(unsigned char)*a-(unsigned char)*b:0;}
char*strncpy(char*d,const char*s,size_t n){char*r=d;while(n&&*s){*d++=*s++;n--;}while(n--)*d++=0;return r;}
char*strrchr(const char*s,int c){const char*r=0;do{if(*s==c)r=s;}while(*s++);return(char*)r;}
char*strstr(const char*s,const char*t){if(!*t)return(char*)s;for(;*s;s++){const char*a=s,*b=t;while(*a&&*a==*b){a++;b++;}if(!*b)return(char*)s;}return 0;}
char*bdx_font_getenv(const char*s){return 0;}
long bdx_font_strtol(const char*s,char**end,int base){const char*start=s;while(*s==' '||*s=='\t')s++;int neg=*s=='-';if(*s=='-'||*s=='+')s++;if(!base)base=*s=='0'?8:10;if(base<2||base>36){if(end)*end=(char*)start;return 0;}unsigned long v=0;int any=0;for(;;){int d=*s>='0'&&*s<='9'?*s-'0':*s>='a'&&*s<='z'?*s-'a'+10:*s>='A'&&*s<='Z'?*s-'A'+10:99;if(d>=base)break;any=1;if(v>((unsigned long)LONG_MAX-d)/base)v=LONG_MAX;else v=v*base+d;s++;}if(end)*end=(char*)(any?s:start);return neg?-(long)v:(long)v;}
void bdx_font_qsort(void*base,size_t n,size_t size,int(*cmp)(const void*,const void*)){
    /* Shell sort avoids recursion or allocation on malformed font input. */
    unsigned char*p=base;if(!size||n>((size_t)-1)/size)return;
    for(size_t gap=n/2;gap;gap/=2)for(size_t i=gap;i<n;i++)for(size_t j=i;j>=gap&&cmp(p+(j-gap)*size,p+j*size)>0;j-=gap)for(size_t k=0;k<size;k++){unsigned char t=p[(j-gap)*size+k];p[(j-gap)*size+k]=p[j*size+k];p[j*size+k]=t;}
}
FT_Error FT_Stream_Open(FT_Stream stream,const char*path){return FT_Err_Cannot_Open_Resource;}
