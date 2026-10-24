/* Native port of 2048's move/merge rules. Gabriele Cirulli, MIT; see
 * third_party/2048/LICENSE.txt. BDX UI, persistence and controls are GPL-2.0+. */
#include "common.h"
typedef struct {u32 magic,score,random;u8 cells[16];}State;
static State state,undo;static int can_undo;static BdsApi*api;static const char*status;
static int index_at(int line,int pos,int dir){return dir==0?line*4+pos:dir==1?line*4+3-pos:dir==2?pos*4+line:(3-pos)*4+line;}
static int move(int dir){
    int changed=0;for(int line=0;line<4;line++){u8 compact[4]={0},result[4]={0};int n=0,k=0;
        for(int p=0;p<4;p++){u8 v=state.cells[index_at(line,p,dir)];if(v)compact[n++]=v;}
        for(int p=0;p<n;p++){u8 v=compact[p];if(p+1<n&&v==compact[p+1]&&v<30){v++;p++;u32 add=1u<<v;state.score=state.score>0xffffffffu-add?0xffffffffu:state.score+add;}result[k++]=v;}
        for(int p=0;p<4;p++){int i=index_at(line,p,dir);if(state.cells[i]!=result[p])changed=1;state.cells[i]=result[p];}
    }return changed;
}
static u32 random_next(void){state.random=state.random*1664525u+1013904223u;return state.random;}
static void add_tile(void){int empty[16],n=0;for(int i=0;i<16;i++)if(!state.cells[i])empty[n++]=i;if(n){int i=empty[random_next()%n];state.cells[i]=random_next()%10?1:2;}}
static void fresh(void){state=(State){0x31584442,0,api->ticks(),{0}};add_tile();add_tile();can_undo=0;status="Join matching tiles to reach 2048.";}
static void save(void){status=api->write("/PUZZLE.SAV",&state,sizeof(state))?"Save unavailable on this volume.":"Game saved to /PUZZLE.SAV";api->print(status);api->print("\n");}
static int valid(const State*s){if(s->magic!=0x31584442)return 0;for(int i=0;i<16;i++)if(s->cells[i]>30)return 0;return 1;}
static void draw(void){
    static const u32 colors[]={0xc4d0da,0xdcecf1,0xc1e3f2,0x8dcae5,0x58afd5,0x2389bc,0x176991,0x285dad,0x414eb3,0x6545ac,0x8a4c9d,0xb55085};
    api->rect(0,0,800,600,0x162a3c);api->text(36,28,"2048 | BDX native edition",0xffffff);char score[16];itoa(score,state.score>0x7fffffffu?0x7fffffff:state.score);api->text(560,28,"Score",0xa5d6ef);api->text(616,28,score,0xffffff);
    api->rect(176,64,448,448,0x0c1e30);for(int y=0;y<4;y++)for(int x=0;x<4;x++){int v=state.cells[y*4+x];api->rect(184+x*110,72+y*110,102,102,colors[v>11?11:v]);if(v){char s[16];itoa(s,1u<<v);api->text(235+x*110-len(s)*4,116+y*110,s,v<4?0x163448:0xffffff);}}
    int won=0,possible=0;for(int i=0;i<16;i++){int v=state.cells[i];if(v>=11)won=1;if(!v||(v<30&&((i%4<3&&v==state.cells[i+1])||(i<12&&v==state.cells[i+4]))))possible=1;}
    api->text(36,530,!possible?"No moves left. Press N for a new game.":won?"2048 reached! Keep playing, or press N for a new game.":status,0xe1edf5);
    api->text(36,558,"Arrows move | U undo | N new | F2 save | Esc save + exit",0xb5d9ec);api->present();
}
int app_main(BdsApi*a,int argc,char**argv){
    api=a;State loaded;int f=a->find("/PUZZLE.SAV");if(a->size(f)==sizeof(loaded)&&a->read(f,0,&loaded,sizeof(loaded))==sizeof(loaded)&&valid(&loaded)){state=loaded;status="Saved game restored.";a->print("2048 restored\n");}else fresh();
    a->print("2048 ready\n");draw();for(;;){int k=a->key(0),dir=k==KEY_LEFT?0:k==KEY_RIGHT?1:k==KEY_UP?2:k==KEY_DOWN?3:-1;
        if(k==27||k==KEY_F10){save();return 0;}if(dir>=0){State before=state;if(move(dir)){undo=before;can_undo=1;add_tile();status="Move complete.";a->print("2048 move\n");}}
        else if(k=='u'||k=='U'){if(can_undo){state=undo;can_undo=0;status="Move undone.";}}
        else if(k=='n'||k=='N')fresh();else if(k==KEY_F1+1)save();if(k)draw();a->sleep(15);
    }
}
