#include <assert.h>
#include <stdio.h>
#include "../apps/game.c"
int main(void){
    seed=17;score=over=0;snake_init();assert(snake_hit(-1,0));assert(snake_hit(sx[1],sy[1]));fx=11;fy=10;snake_step();assert(score==10&&sn==5&&sx[0]==11);
    snake_init();sx[0]=29;sd=1;snake_step();assert(over);
    score=over=0;pong_init();py=200;bx=63;by=210;bvx=-5;pong_step();assert(bvx==5&&score==1);bx=63;by=100;bvx=-5;pong_step();assert(over);
    score=over=0;blocks_init();shape=1;assert(blocks_hit(-2,0,0));for(int x=0;x<10;x++)board[19][x]=1;tx=3;ty=17;rotation=0;blocks_step();blocks_step();assert(score>=100);
    score=over=0;mines_init();int n=0;for(int y=0;y<10;y++)for(int x=0;x<10;x++)n+=mines[y][x];assert(n==15);for(int y=0;y<10;y++)for(int x=0;x<10;x++)if(mines[y][x]){uncover(x,y);assert(over);goto hit;}
hit:score=over=0;arcade_init();enemyx[0]=100;enemyy[0]=100;enemyv[0]=0;shotx=110;shoty=120;arcade_step();assert(score==10&&shoty==-1);
    puts("PASS: snake collision/growth, pong bounce/miss, block bounds/line clear, mine layout/collision, arcade hit");return 0;
}
