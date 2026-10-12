// Native BDS-OS port of InfoNES. Original integration: GPL-2.0-or-later.
// Combined with the GPL-3.0 mapper 90 implementation under GPL-3.0.
#include "bds.h"
#include "InfoNES.h"
#include "InfoNES_System.h"
#include <stdarg.h>

static BdsApi *bds;
static u32 pixels[256*240];
static u32 frames,deadline,rom_hash;
static DWORD pad,pad2;
static int quitting,resetting,pause,save_failed;
static int audio_rate=44100,opening_rom;
static int audio_dc;
static char next_rom[256];
static char save_path[256];
WORD NesPalette[64];
static const u32 palette[64]={
  0x707070,0x201888,0x0000a8,0x400098,0x880070,0xa80010,0xa00000,0x780800,
  0x402800,0x004000,0x005000,0x003810,0x183858,0,0,0,
  0xb8b8b8,0x0070e8,0x2038e8,0x8000f0,0xb800b8,0xe00058,0xd82800,0xc84808,
  0x887000,0x009000,0x00a800,0x009038,0x008088,0,0,0,
  0xf8f8f8,0x38b8f8,0x5890f8,0x4088f8,0xf078f8,0xf870b0,0xf87060,0xf89838,
  0xf0b838,0x80d010,0x48d848,0x58f898,0x00e8d8,0,0,0,
  0xf8f8f8,0xa8e0f8,0xc0d0f8,0xd0c8f8,0xf8c0f8,0xf8c0d8,0xf8b8b0,0xf8d8a8,
  0xf8e0a0,0xe0f8a0,0xa8f0b8,0xb0f8c8,0x98f8f0,0,0,0
};
static int length(const char*s){int n=0;while(s[n])n++;return n;}
static void copy(char*d,const char*s){while((*d++=*s++));}
static void printnum(u32 n){char out[16],rev[16];int i=0,j=0;do{rev[i++]='0'+n%10;n/=10;}while(n);while(i)out[j++]=rev[--i];out[j]=0;bds->print(out);}
static void lognum(const char*s,u32 n){bds->print(s);printnum(n);bds->print("\n");}
static u32 ram_size(BYTE n){return n?64u<<n:0;}
void *InfoNES_MemoryCopy(void*d,const void*s,int n){BYTE*a=(BYTE*)d;const BYTE*b=(const BYTE*)s;while(n-->0)*a++=*b++;return d;}
void *InfoNES_MemorySet(void*d,int c,int n){BYTE*a=(BYTE*)d;while(n-->0)*a++=c;return d;}
extern "C" void *memcpy(void*d,const void*s,usize n){return InfoNES_MemoryCopy(d,s,n);}
extern "C" void *memset(void*d,int c,usize n){return InfoNES_MemorySet(d,c,n);}
static u32 hash_sram(){u32 h=2166136261u;for(int i=0;i<SRAM_SIZE;i++)h=(h^SRAM[i])*16777619u;return h;}
static void save_sram(){
  if(!(NesHeader.byInfo1&2)||save_failed||hash_sram()==rom_hash)return;
  if(bds->write(save_path,SRAM,SRAM_SIZE)){bds->print("NES: battery save failed; disk full or read-only\n");save_failed=1;}
  else{rom_hash=hash_sram();bds->print("NES battery saved\n");}
}
int InfoNES_ReadRom(const char*path){
  int f=bds->find(path),size=bds->size(f);
  if(size<16||length(path)>250||bds->read(f,0,&NesHeader,16)!=16){bds->print("NES: missing or short ROM\n");return -1;}
  if(NesHeader.byID[0]!='N'||NesHeader.byID[1]!='E'||NesHeader.byID[2]!='S'||NesHeader.byID[3]!=26){bds->print("NES: expected an iNES .NES image\n");return -1;}
  int nes2=(NesHeader.byInfo2&12)==8;
  if(nes2){
    // This core uses 8-bit mapper numbers, linear ROM sizes and 8 KiB RAM.
    if(NesHeader.byReserve[0]||NesHeader.byReserve[1]||
       (NesHeader.byReserve[2]&15)>7||(NesHeader.byReserve[2]>>4)>7||
       (NesHeader.byReserve[3]&15)>7||(NesHeader.byReserve[3]>>4)>7||
       ram_size(NesHeader.byReserve[2]&15)+ram_size(NesHeader.byReserve[2]>>4)>8192||
       ram_size(NesHeader.byReserve[3]&15)+ram_size(NesHeader.byReserve[3]>>4)>8192||
       (NesHeader.byReserve[4]&3)==1||(NesHeader.byReserve[4]&3)==3||
       NesHeader.byReserve[5]||NesHeader.byReserve[6]||NesHeader.byReserve[7]>1){
      bds->print("NES: unsupported extended NES 2.0 hardware\n");return -1;
    }
    InfoNES_MemorySet(NesHeader.byReserve,0,8);
    NesHeader.byInfo2&=0xf3; // Normalize for InfoNES's original iNES mapper decoder.
    bds->print("NES: compatible NES 2.0 header\n");
  }
  // Original iNES dump tools wrote DiskDude! into bytes 7-15.
  int dirty=0;for(int i=4;i<8;i++)dirty|=NesHeader.byReserve[i];
  if(!dirty&&(NesHeader.byInfo2&3)){bds->print("NES: VS/PlayChoice images are not supported\n");return -1;}
  if(dirty)bds->print("NES: ignoring dirty legacy mapper high bits\n");
  u32 prg=NesHeader.byRomSize*16384u,chr=NesHeader.byVRomSize*8192u,offset=16+((NesHeader.byInfo1&4)?512:0);
  if(!prg||offset+prg+chr>(u32)size){bds->print("NES: truncated or empty ROM\n");return -1;}
  ROM=(BYTE*)bds->alloc(prg);VROM=chr?(BYTE*)bds->alloc(chr):0;
  if(!ROM||(chr&&!VROM)){bds->print("NES: insufficient memory\n");InfoNES_ReleaseRom();return -1;}
  InfoNES_MemorySet(SRAM,0,SRAM_SIZE);
  if((NesHeader.byInfo1&4)&&bds->read(f,16,SRAM+0x1000,512)!=512){InfoNES_ReleaseRom();return -1;}
  if(bds->read(f,offset,ROM,prg)!=(int)prg||(chr&&bds->read(f,offset+prg,VROM,chr)!=(int)chr)){InfoNES_ReleaseRom();return -1;}
  copy(save_path,path);int n=length(save_path);if(n>=4&&save_path[n-4]=='.')copy(save_path+n-4,".SAV");else copy(save_path+n,".SAV");
  int sf=bds->find(save_path),ss=bds->size(sf);
  if((NesHeader.byInfo1&2)&&ss==SRAM_SIZE){if(bds->read(sf,0,SRAM,SRAM_SIZE)==SRAM_SIZE)bds->print("NES battery loaded\n");else{bds->print("NES: battery read failed\n");InfoNES_ReleaseRom();return -1;}}
  else if((NesHeader.byInfo1&2)&&ss>=0){bds->print("NES: invalid battery save size\n");InfoNES_ReleaseRom();return -1;}
  rom_hash=hash_sram();save_failed=0;bds->print("NES ROM: ");bds->print(path);bds->print("\n");return 0;
}
void InfoNES_ReleaseRom(){if(ROM)bds->free(ROM);if(VROM)bds->free(VROM);ROM=VROM=0;}
int InfoNES_Menu(){return -1;}
void InfoNES_DebugPrint(char*s){bds->print(s);}
void InfoNES_MessageBox(char*s,...){
  va_list ap;va_start(ap,s);bds->print("NES: ");
  while(*s){if(s[0]=='%'&&s[1]=='d'){printnum(va_arg(ap,int));s+=2;}else{char c[2]={*s++,0};bds->print(c);}}
  va_end(ap);
}
static void input(){
  int e;while((e=bds->keyevent())){
    int k=e&65535,up=e&KEY_RELEASE;u32 bit=0,bit2=0;
    if(k==KEY_OPEN&&bds->version>=2&&!bds->opened_path(next_rom,sizeof(next_rom))){opening_rom=quitting=1;continue;}
    if(k>='A'&&k<='Z')k+=32;
    if(k==KEY_UP)bit=16;else if(k==KEY_DOWN)bit=32;else if(k==KEY_LEFT)bit=64;else if(k==KEY_RIGHT)bit=128;
    else if(k=='x')bit=1;else if(k=='z')bit=2;else if(k==9)bit=4;else if(k==13)bit=8;
    if(k=='i')bit2=16;else if(k=='k')bit2=32;else if(k=='j')bit2=64;else if(k=='l')bit2=128;
    else if(k=='c')bit2=1;else if(k=='v')bit2=2;else if(k=='b')bit2=4;else if(k==' ')bit2=8;
    if(up)pad&=~bit;else pad|=bit;
    if(up)pad2&=~bit2;else pad2|=bit2;
    if(!up&&(k==27||k==KEY_F10))quitting=1;
    if(!up&&k=='r')resetting=1;
    if(!up&&k=='p'){pause=!pause;deadline=bds->ticks();}
    if(!up&&bit)lognum("NES controller: ",pad);
    if(!up&&bit2)lognum("NES controller 2: ",pad2);
  }
  while(bds->key(0));
}
void InfoNES_PadState(DWORD*a,DWORD*b,DWORD*s){input();*a=pad;*b=pad2;*s=(quitting||resetting)?PAD_SYS_QUIT:0;}
void InfoNES_LoadFrame(){
  for(int i=0;i<256*240;i++)pixels[i]=palette[WorkFrame[i]&63];
  bds->blit(pixels,256,240);frames++;
  if(frames==1||frames==120)lognum("NES frame ",frames);
  input();while(pause&&!quitting&&!resetting){bds->sleep(20);input();}
  // PIT has 10 ms resolution; use a fractional 60 Hz deadline.
  deadline+=((frames%3)==0?16:17);u32 now=bds->ticks();
  if((int)(deadline-now)>0)bds->sleep(deadline-now);else if((int)(now-deadline)>100)deadline=now;
  if(frames%300==0)save_sram();
}
void InfoNES_Wait(){}
void InfoNES_SoundInit(){}
int InfoNES_SoundOpen(int,int rate){audio_rate=rate;audio_dc=0;return 1;}
void InfoNES_SoundClose(){}
void InfoNES_SoundOutput(int n,BYTE*a,BYTE*b,BYTE*c,BYTE*d,BYTE*e){
  if(bds->version<2||!bds->audio_available()||n<1||n>1024)return;short pcm[1024];
  for(int i=0;i<n;i++){int level=((int)a[i]+b[i]+c[i]+d[i]+e[i])*64;audio_dc+=(level-audio_dc)/256;int v=level-audio_dc;if(v>32767)v=32767;if(v<-32768)v=-32768;pcm[i]=v;}
  bds->audio_write(pcm,n,audio_rate,1);
}

struct Choice{char path[64],title[96];};
static Choice choices[64];
static int picker(char*path){
  if(bds->version>=2){bds->print("NES library ready\n");return bds->pick_file(".NES",path,256);}
  int f=bds->find("/NES/ROMS.TXT"),size=bds->size(f),count=0;
  if(size<=0||size>16384){bds->print("NES: no ROM library. Use NES /path.nes or tools/nes_import.py.\n");return -1;}
  char*data=(char*)bds->alloc(size+1);if(!data)return -1;
  if(bds->read(f,0,data,size)!=size){bds->free(data);return -1;}data[size]=0;
  char*s=data;while(*s&&count<64){char*line=s;while(*s&&*s!='\n')s++;if(*s)*s++=0;char*t=line;while(*t&&*t!='\t')t++;if(!*t)continue;*t++=0;int n=length(t);if(n&&t[n-1]=='\r')t[n-1]=0;
    if(length(line)<64&&length(t)<96){copy(choices[count].path,line);copy(choices[count++].title,t);}}
  bds->free(data);if(!count){bds->print("NES: empty ROM library\n");return -1;}
  int selected=0;bds->print("NES library ready\n");
  int visible=(bds->height-180)/26;if(visible<1)visible=1;if(visible>15)visible=15;
  for(;;){int first=selected>=visible?selected-visible+1:0;
    bds->rect(0,0,bds->width,bds->height,0x171d1c);bds->text(32,24,"NES LIBRARY",0xffffff);
    bds->text(32,48,"Up/Down select | Enter play | ESC return",0x95bda4);
    for(int i=first;i<count&&i<first+visible;i++){int y=88+(i-first)*26;if(i==selected)bds->rect(24,y-5,bds->width-48,24,0x36594b);bds->text(32,y,choices[i].title,0xffffff);}
    bds->text(32,bds->height-52,"Play: arrows | X A | Z B | Enter Start | Tab Select",0x95bda4);
    bds->text(32,bds->height-30,"P pause | R reset | ESC/F10 exit | silent audio",0x95bda4);bds->present();
    int e=bds->keyevent();while(bds->key(0));if(!(e&KEY_RELEASE)){if(e==27)return -1;if(e==KEY_UP&&selected)selected--;if(e==KEY_DOWN&&selected+1<count)selected++;if(e==13){copy(path,choices[selected].path);return 0;}}bds->sleep(20);
  }
}
extern "C" int app_main(BdsApi*api,int argc,char**argv){
  bds=api;for(int i=0;i<64;i++)NesPalette[i]=i;
  char path[256];if(argc>=2){if(length(argv[1])>250){bds->print("NES: ROM path too long\n");return 1;}copy(path,argv[1]);}else if(picker(path))return 0;
load_rom:
  quitting=opening_rom=resetting=pause=0;InfoNES_Init();if(InfoNES_Load(path)){InfoNES_Fin();return 1;}
  lognum("NES mapper: ",MapperNo);bds->print(bds->version>=2&&bds->audio_available()?"NES native emulation started (PCM audio)\n":"NES native emulation started (silent)\n");deadline=bds->ticks();
  do{resetting=0;pad=pad2=0;InfoNES_Cycle();save_sram();if(resetting&&!quitting){InfoNES_Reset();deadline=bds->ticks();bds->print("NES reset\n");}}while(resetting&&!quitting);
  InfoNES_Fin();if(opening_rom){copy(path,next_rom);bds->audio_stop();goto load_rom;}bds->print("NES exit to BDS-OS\n");return save_failed?1:0;
}
