#include "bds.h"
#define REQUIRE(x) do{if(!(x)){a->print("MODERN PROBE FAIL: " #x "\n");return 1;}}while(0)
int app_main(BdsApi*a,int argc,char**argv){
    if(argc>1){
        u32*p=a->alloc(1024*768*4);REQUIRE(p);
        for(int y=0;y<768;y++)for(int x=0;x<1024;x++)p[y*1024+x]=((x&255)<<16)|((y&255)<<8)|0x5a;
        a->fullscreen(1);a->blit(p,1024,768);a->print("MODERN BLIT ready\n");
        while(a->key(0)!=27)a->sleep(10);a->free(p);return 0;
    }
    REQUIRE(a->version>=7);char text[4096],small[4]={'x','x','x','x'};BdsSystemInfo info;
    REQUIRE(!a->clipboard_set("shared clipboard",16));REQUIRE(a->clipboard_get(small,sizeof(small))==-1);REQUIRE(small[0]=='x');
    REQUIRE(a->clipboard_set(text,sizeof(text))==-1);REQUIRE(a->clipboard_get(text,sizeof(text))==16);REQUIRE(text[0]=='s'&&text[16]==0);
    REQUIRE(a->system_info(&info,sizeof(info)-1)==-1);REQUIRE(!a->system_info(&info,sizeof(info)));REQUIRE(info.arch_bits==sizeof(void*)*8&&info.abi>=7);
    u32 used=info.heap_used;void*p=a->alloc(65536);REQUIRE(p);a->system_info(&info,sizeof(info));REQUIRE(info.heap_used>=used+65536);a->free(p);
    u32 begin=a->ticks();a->sleep(50);REQUIRE(a->ticks()-begin>=50&&a->ticks()-begin<1000);
    a->print("MODERN PROBE PASS: clipboard bounds, preservation, architecture, heap and timer\n");return 0;
}
