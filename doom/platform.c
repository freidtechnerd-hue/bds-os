#include "libc.h"
#include "bds.h"
#include "doomgeneric.h"
#include "doomstat.h"
// Key constants in bds.h and doomkeys.h intentionally use different domains.
BdsApi*bds;static u32 frames,lastx,lasty;static int reported,loading;static char wad_path[256];
void doom_loading(const char*stage,u32 done,u32 total){
    if(!loading)return;
    bds->rect(0,0,800,600,0x171f1d);bds->text(48,160,"DOOM",0xe2bb75);bds->text(48,198,"Loading game data and preparing the level...",0xe1e6dc);
    bds->text(48,234,wad_path,0x94bd9f);bds->text(48,276,stage,0xe1e6dc);bds->rect(48,314,704,14,0x35473c);
    if(total)bds->rect(48,314,704*(done/1024)/(total/1024?total/1024:1),14,0xa4d3a6);
    else bds->rect(48+(bds->ticks()/30)%624,314,80,14,0xa4d3a6);
    bds->text(48,354,"Please wait. The game will start in this window.",0x9cac9e);bds->present();
}
void DG_Init(void){bds->print("DOOM platform: BDS framebuffer, filesystem, PIT and PS/2\n");}
void DG_DrawFrame(void){bds->blit(DG_ScreenBuffer,DOOMGENERIC_RESX,DOOMGENERIC_RESY);frames++;if(frames==1)bds->print("DOOM frame 1\n");if(frames==100)bds->print("DOOM frame 100\n");if(gamestate==GS_LEVEL&&players[consoleplayer].mo){u32 x=players[consoleplayer].mo->x,y=players[consoleplayer].mo->y;if(!reported){bds->print("DOOM playable level active\n");lastx=x;lasty=y;reported=1;}else if(reported==1&&(x!=lastx||y!=lasty)){bds->print("DOOM player moved\n");reported=2;}}}
void DG_SleepMs(uint32_t ms){bds->sleep(ms);}uint32_t DG_GetTicksMs(void){return bds->ticks();}
int DG_GetKey(int*pressed,unsigned char*out){int e=bds->keyevent();if(!e)return 0;*pressed=!(e&KEY_RELEASE);int k=e&0xffff;if(k==KEY_OPEN&&*pressed)bds->exit(42);if(k==KEY_F1+9&&*pressed)exit(0);if(k==KEY_UP||k=='w')k=0xad;else if(k==KEY_DOWN||k=='s')k=0xaf;else if(k==KEY_LEFT||k=='a')k=0xac;else if(k==KEY_RIGHT||k=='d')k=0xae;else if(k==' ')k=0xa2;else if(k=='f')k=0xa3;else if(k>=KEY_F1&&k<=KEY_F1+8)k=0xbb+k-KEY_F1;if(k==8)k=0x7f;*out=k;return 1;}
void DG_SetWindowTitle(const char*title){bds->print("DOOM title: ");bds->print(title);bds->print("\n");}
int app_main(BdsApi*api,int argc,char**argv){
    bds=api;int iw=0;for(int i=1;i+1<argc;i++)if(!strcmp(argv[i],"-iwad")){strcpy(wad_path,argv[i+1]);iw=1;break;}
    char*args[20];if(!iw){bds->print("DOOM WAD selector ready\n");if(bds->pick_file(".WAD",wad_path,sizeof(wad_path)))return 0;for(int i=0;i<argc&&i<16;i++)args[i]=argv[i];args[argc++]="-iwad";args[argc++]=wad_path;argv=args;}
    int f=bds->find(wad_path);u8 h[12];int size=bds->size(f);if(size<12||bds->read(f,0,h,12)!=12||memcmp(h,"IWAD",4)){bds->print("DOOM: select a valid IWAD game file\n");return 1;}
    loading=1;doom_loading("Starting engine",0,0);bds->print("DOOM loading screen ready\n");doomgeneric_Create(argc,argv);loading=0;for(;;)doomgeneric_Tick();
}
