#include <assert.h>
#include <stdio.h>
#include "../apps/puzzle.c"
int main(void){
    state=(State){.cells={1,1,1,1}};assert(move(0));assert(state.cells[0]==2&&state.cells[1]==2&&!state.cells[2]&&state.score==8);
    state=(State){.cells={1,1,2,0}};assert(move(0));assert(state.cells[0]==2&&state.cells[1]==2&&state.score==4);
    state=(State){.cells={1,0,1,1}};assert(move(1));assert(state.cells[3]==2&&state.cells[2]==1&&!state.cells[1]);
    state=(State){.cells={1,0,0,0,1,0,0,0,1,0,0,0,1}};assert(move(2));assert(state.cells[0]==2&&state.cells[4]==2&&!state.cells[8]);
    state=(State){.cells={1,2,3,4,2,3,4,5,3,4,5,6,4,5,6,7}};assert(!move(0));assert(!move(1));assert(!move(2));assert(!move(3));
    state=(State){.cells={30,30}};assert(!move(0));
    puts("PASS 2048 four directions, merge-once, blocked board and exponent bound");
}
