#include "bds.h"
int app_main(BdsApi*a,int argc,char**argv){
    char data[256];if(a->version<2)return 1;
    const char*names[]={"DATA/EXAMPLE.BIN","DATA/EXAMPLE.DLL","DATA/EXAMPLE.DCL"};
    for(int i=0;i<3;i++)if(a->resource(names[i],data,sizeof(data))<=0)return 2;
    if(a->resource("DATA/MISSING.BIN",data,sizeof(data))>=0)return 3;
    if(a->font("/FONTS/BOLD.PSF"))return 4;if(!a->font("/DATA/EXAMPLE.BIN"))return 5;
    a->print("FEATURES: resources and custom font passed\n");
    for(;;){a->rect(0,0,a->width,a->height,0x17251d);a->text(24,40,"FUTURE RUN - custom sidecar icon",0xe8d49a);a->text(24,80,"BIN / DLL / DCL companion resources loaded",0x95bda4);a->present();if(a->key(0)==27)return 0;a->sleep(20);}
}
