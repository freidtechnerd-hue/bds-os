#include "bds.h"
static int same(const char*a,const char*b,int n){while(n--)if(*a++!=*b++)return 0;return 1;}
int app_main(BdsApi*a,int argc,char**argv){
    int root=a->find("/README.TXT");char before[32],after[32],chosen[256],data[64];if(a->read(root,0,before,32)!=32)return 1;
    a->print("VOLUME PROBE: choose a TXT file on another drive\n");
    if(a->pick_file(".TXT",chosen,sizeof(chosen)))return 2;
    if(a->read(root,0,after,32)!=32||!same(before,after,32))return 3;
    if(a->read(a->find(chosen),0,data,19)!=19||!same(data,"secondary disk text",19))return 4;
    if(a->write("V1:/WRITTEN.TXT","cross-volume write",18))return 5;
    if(a->read(a->find("/README.TXT"),0,after,32)!=32||!same(before,after,32))return 6;
    if(a->resource("DATA/EXAMPLE.BIN",data,sizeof(data))<1)return 7;
    a->print("VOLUME PROBE: handles, picker restore, write and resources passed\n");
    for(;;){a->rect(0,0,800,600,0x112233);a->text(20,20,"Embedded icon and stable system resources",0xffffff);a->present();if(a->key(0)==27)return 0;a->sleep(30);}
}
