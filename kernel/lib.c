#include "os.h"
void *memcpy(void*d,const void*s,usize n){u8*a=d;const u8*b=s;while(n--)*a++=*b++;return d;}
void *memmove(void*d,const void*s,usize n){u8*a=d;const u8*b=s;if(a<b)return memcpy(d,s,n);while(n){n--;a[n]=b[n];}return d;}
void *memset(void*d,int c,usize n){u8*a=d;while(n--)*a++=c;return d;}
int memcmp(const void*a,const void*b,usize n){const u8*x=a,*y=b;while(n--){if(*x!=*y)return *x-*y;x++;y++;}return 0;}
usize strlen(const char*s){usize n=0;while(s[n])n++;return n;}
int strcmp(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return (u8)*a-(u8)*b;}
static char upper(char c){return c>='a'&&c<='z'?c-32:c;}
int stricmp(const char*a,const char*b){while(*a&&upper(*a)==upper(*b)){a++;b++;}return upper(*a)-upper(*b);}
char*strcpy(char*d,const char*s){char*r=d;while((*d++=*s++));return r;}
int atoi(const char*s){int v=0,neg=0;if(*s=='-'){neg=1;s++;}while(*s>='0'&&*s<='9')v=v*10+*s++-'0';return neg?-v:v;}
void number(char*d,u32 v){char b[16];int i=0;do{b[i++]='0'+v%10;v/=10;}while(v);while(i)*d++=b[--i];*d=0;}
/* Payloads must remain 16-byte aligned for native SSE2 code. */
typedef struct Block{u32 size,free;struct Block*next;u32 reserved[sizeof(void*)==8?4:1];}Block;
static Block *heap;
void heap_reset(void){heap=(Block*)0x1000000;heap->size=0x4000000-sizeof(Block);heap->free=1;heap->next=0;}
void *alloc(usize size){if(!size||size>0x3ffffff)return 0;size=(size+15)&~15u;for(Block*b=heap;b;b=b->next){if(b->free&&b->size>=size){if(b->size>size+sizeof(Block)+16){Block*n=(Block*)((u8*)(b+1)+size);n->size=b->size-size-sizeof(Block);n->free=1;n->next=b->next;b->next=n;b->size=size;}b->free=0;return b+1;}}return 0;}
void dealloc(void*p){if(!p)return;for(Block*b=heap;b;b=b->next)if(b+1==p){b->free=1;break;}for(Block*b=heap;b&&b->next;)if(b->free&&b->next->free){b->size+=sizeof(Block)+b->next->size;b->next=b->next->next;}else b=b->next;}
u32 heap_used(void){u32 n=0;for(Block*b=heap;b;b=b->next)if(!b->free)n+=b->size;return n;}
