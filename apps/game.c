#include "common.h"
static BdsApi*a;static unsigned seed;static int score,over;
static unsigned rnd(void){seed=seed*1664525+1013904223;return seed;}
static void background(const char*name){a->rect(0,0,a->width,a->height,0x17211d);a->text(24,20,name,0xcce3bf);a->text(24,a->height-26,"Arrows / WASD | Space | R restart | ESC exit",0xc6b270);char b[16];itoa(b,score);a->text(a->width-160,20,"SCORE",0xcce3bf);a->text(a->width-96,20,b,0xcce3bf);if(over)a->text(300,50,"GAME OVER - R restarts",0xffaa80);}
static int sx[400],sy[400],sn,sd,fx,fy;
static void snake_init(void){sn=4;sd=1;for(int i=0;i<sn;i++){sx[i]=10-i;sy[i]=10;}fx=18;fy=10;}
static int snake_hit(int x,int y){if(x<0||y<0||x>=30||y>=20)return 1;for(int i=0;i<sn-1;i++)if(sx[i]==x&&sy[i]==y)return 1;return 0;}
static void snake_step(void){int dx[]={0,1,0,-1},dy[]={-1,0,1,0};int x=sx[0]+dx[sd],y=sy[0]+dy[sd];if(snake_hit(x,y)){over=1;return;}if(x==fx&&y==fy){score+=10;if(sn<399)sn++;int busy;do{fx=rnd()%30;fy=rnd()%20;busy=0;for(int i=0;i<sn-1;i++)if(sx[i]==fx&&sy[i]==fy)busy=1;}while(busy);}for(int i=sn-1;i>0;i--){sx[i]=sx[i-1];sy[i]=sy[i-1];}sx[0]=x;sy[0]=y;}
static int px,py,bx,by,bvx,bvy,ai;
static void pong_init(void){px=200;py=200;bx=360;by=260;bvx=5;bvy=3;ai=200;}
static void pong_step(void){bx+=bvx;by+=bvy;if(by<82||by>470)bvy=-bvy;if(ai+40<by)ai+=4;else if(ai+40>by)ai-=4;if(bx<62){if(by>=py&&by<=py+80){bvx=-bvx;score++;}else over=1;}if(bx>710){if(by>=ai&&by<=ai+80)bvx=-bvx;else{score+=5;bx=360;}}}
static u8 board[20][10];static int tx,ty,shape,rotation;
static const u16 pieces[7][4]={{0x0f00,0x2222,0x0f00,0x2222},{0x6600,0x6600,0x6600,0x6600},{0x4e00,0x4640,0x0e40,0x4c40},{0x8e00,0x6440,0x0e20,0x44c0},{0x2e00,0x4460,0x0e80,0xc440},{0x6c00,0x4620,0x6c00,0x4620},{0xc600,0x2640,0xc600,0x2640}};
static int blocks_hit(int x,int y,int r){u16 p=pieces[shape][r];for(int j=0;j<4;j++)for(int i=0;i<4;i++)if(p&(0x8000>>(j*4+i))){int xx=x+i,yy=y+j;if(xx<0||xx>=10||yy>=20||(yy>=0&&board[yy][xx]))return 1;}return 0;}
static void spawn(void){tx=3;ty=0;rotation=0;shape=rnd()%7;if(blocks_hit(tx,ty,rotation))over=1;}
static void blocks_init(void){for(int j=0;j<20;j++)for(int i=0;i<10;i++)board[j][i]=0;spawn();}
static void blocks_step(void){if(!blocks_hit(tx,ty+1,rotation)){ty++;return;}u16 p=pieces[shape][rotation];for(int j=0;j<4;j++)for(int i=0;i<4;i++)if(p&(0x8000>>(j*4+i)))board[ty+j][tx+i]=shape+1;for(int y=19;y>=0;y--){int full=1;for(int x=0;x<10;x++)if(!board[y][x])full=0;if(full){for(int yy=y;yy>0;yy--)for(int x=0;x<10;x++)board[yy][x]=board[yy-1][x];for(int x=0;x<10;x++)board[0][x]=0;score+=100;y++;}}spawn();}
static u8 mines[10][10],opened[10][10],flags[10][10];static int mx,my;
static int count(int x,int y){int n=0;for(int j=-1;j<=1;j++)for(int i=-1;i<=1;i++)if(x+i>=0&&x+i<10&&y+j>=0&&y+j<10)n+=mines[y+j][x+i];return n;}
static void uncover(int x,int y){if(x<0||x>=10||y<0||y>=10||opened[y][x]||flags[y][x])return;opened[y][x]=1;if(mines[y][x]){over=1;return;}score++;if(!count(x,y))for(int j=-1;j<=1;j++)for(int i=-1;i<=1;i++)if(i||j)uncover(x+i,y+j);if(score==85)over=1;}
static void mines_init(void){mx=my=0;for(int j=0;j<10;j++)for(int i=0;i<10;i++)mines[j][i]=opened[j][i]=flags[j][i]=0;for(int n=0;n<15;){int x=rnd()%10,y=(rnd()>>8)%10;if(!mines[y][x]){mines[y][x]=1;n++;}}}
static int ship,shotx,shoty,enemyx[8],enemyy[8],enemyv[8];
static void arcade_init(void){ship=350;shoty=-1;for(int i=0;i<8;i++){enemyx[i]=40+rnd()%650;enemyy[i]=70+rnd()%180;enemyv[i]=(rnd()&1)?3:-3;}}
static void arcade_step(void){if(shoty>=0)shoty-=12;for(int i=0;i<8;i++){enemyx[i]+=enemyv[i];if(enemyx[i]<30||enemyx[i]>720){enemyv[i]=-enemyv[i];enemyy[i]+=20;}if(enemyy[i]>=440)over=1;if(shoty>=enemyy[i]&&shoty<=enemyy[i]+24&&shotx>=enemyx[i]&&shotx<=enemyx[i]+30){score+=10;enemyy[i]=80;enemyx[i]=40+rnd()%650;shoty=-1;}}}
int app_main(BdsApi*api,int argc,char**argv){a=api;seed=a->ticks();int g=0;if(argc>1){if(eq(argv[1],"pong"))g=1;else if(eq(argv[1],"blocks"))g=2;else if(eq(argv[1],"mines"))g=3;else if(eq(argv[1],"arcade"))g=4;else if(!eq(argv[1],"snake")){a->print("Unknown game.\n");return 1;}}
    const char*names[]={"Snake","Pong","Falling Blocks","Mines","Star Arcade"};a->print("GAME started: ");a->print(names[g]);a->print("\n");
restart:score=over=0;snake_init();pong_init();blocks_init();mines_init();arcade_init();u32 last=a->ticks();int frames=0;
    for(;;){int k=a->key(0);if(k==27){a->print("GAME exit\n");return 0;}if(k=='r'||k=='R')goto restart;
        if(!over){if(g==0){if((k==KEY_UP||k=='w')&&sd!=2)sd=0;if((k==KEY_RIGHT||k=='d')&&sd!=3)sd=1;if((k==KEY_DOWN||k=='s')&&sd!=0)sd=2;if((k==KEY_LEFT||k=='a')&&sd!=1)sd=3;}
            if(g==1){if(k==KEY_UP||k=='w')py-=16;if(k==KEY_DOWN||k=='s')py+=16;if(py<82)py=82;if(py>390)py=390;}
            if(g==2){if((k==KEY_LEFT||k=='a')&&!blocks_hit(tx-1,ty,rotation))tx--;if((k==KEY_RIGHT||k=='d')&&!blocks_hit(tx+1,ty,rotation))tx++;if((k==KEY_UP||k=='w')&&!blocks_hit(tx,ty,(rotation+1)%4))rotation=(rotation+1)%4;if(k==KEY_DOWN||k=='s')blocks_step();if(k==' '){while(!blocks_hit(tx,ty+1,rotation))ty++;blocks_step();}}
            if(g==3){if((k==KEY_LEFT||k=='a')&&mx)mx--;if((k==KEY_RIGHT||k=='d')&&mx<9)mx++;if((k==KEY_UP||k=='w')&&my)my--;if((k==KEY_DOWN||k=='s')&&my<9)my++;if(k==' '||k==13)uncover(mx,my);if(k=='f')flags[my][mx]=!flags[my][mx];}
            if(g==4){if(k==KEY_LEFT||k=='a')ship-=18;if(k==KEY_RIGHT||k=='d')ship+=18;if(ship<30)ship=30;if(ship>720)ship=720;if(k==' '&&shoty<0){shotx=ship+14;shoty=435;}}}
        u32 now=a->ticks(),interval=g==0?130:g==2?450:30;if(now-last>=interval){last=now;if(!over){if(g==0)snake_step();if(g==1)pong_step();if(g==2)blocks_step();if(g==4)arcade_step();}}
        background(names[g]);if(g==0){a->rect(70,80,602,402,0x465847);a->rect(71,81,600,400,0x111a14);for(int i=0;i<sn;i++)a->rect(72+sx[i]*20,82+sy[i]*20,18,18,i?0x519c61:0x98ea98);a->rect(72+fx*20,82+fy*20,18,18,0xdd9977);}
        else if(g==1){a->rect(50,80,680,400,0x111a14);a->rect(54,py,9,80,0x9add9e);a->rect(714,ai,9,80,0xddbc80);a->rect(bx,by,10,10,0xffffff);}
        else if(g==2){a->rect(270,70,202,402,0x101914);for(int j=0;j<20;j++)for(int i=0;i<10;i++)if(board[j][i])a->rect(271+i*20,71+j*20,18,18,0x65af79+board[j][i]*0x120509);u16 p=pieces[shape][rotation];for(int j=0;j<4;j++)for(int i=0;i<4;i++)if(p&(0x8000>>(j*4+i)))a->rect(271+(tx+i)*20,71+(ty+j)*20,18,18,0xe2bc79);}
        else if(g==3){for(int j=0;j<10;j++)for(int i=0;i<10;i++){int x=190+i*34,y=90+j*34;a->rect(x,y,31,31,opened[j][i]?0x526455:0xadb6a5);if(opened[j][i]){char s[2]={'0'+count(i,j),0};if(mines[j][i])s[0]='*';a->text(x+10,y+12,s,0xffffff);}else if(flags[j][i])a->text(x+10,y+12,"F",0x9b332c);}a->rect(190+mx*34,90+my*34,31,2,0xffff80);a->text(190,455,"Space uncover | F flag",0xddc990);}
        else{a->rect(ship,440,30,16,0xa2e3ad);if(shoty>=0)a->rect(shotx,shoty,3,10,0xffec89);for(int i=0;i<8;i++)a->rect(enemyx[i],enemyy[i],30,24,0xba7f76);}
        a->present();if(++frames==100)a->print("GAME frames: 100\n");a->sleep(20);
    }
}
