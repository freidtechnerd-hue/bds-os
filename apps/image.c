#include "libc.h"
#include "bds.h"
#include "lodepng.h"
BdsApi*bds;static char path[256],status[100];static u8*bytes,*rgba;static int width,height,bmp,topdown,step;static u32 offset,row;
static int open_image(const char*p){
    free(bytes);free(rgba);bytes=rgba=0;width=height=0;strcpy(path,p);int f=bds->find(p),n=bds->size(f);strcpy(status,"Unsupported or damaged image");
    if(n<24||n>8*1024*1024)return -1;bytes=malloc(n);if(!bytes)return -1;if(bds->read(f,0,bytes,n)!=n)return -1;
    if(n>=54&&!memcmp(bytes,"BM",2)){
        int w=*(int*)(bytes+18),h=*(int*)(bytes+22);u32 dib=*(u32*)(bytes+14);offset=*(u32*)(bytes+10);u16 bits=*(u16*)(bytes+28);step=bits/8;
        if(dib<40||dib>(u32)n-14||offset<14+dib||*(u16*)(bytes+26)!=1||*(u32*)(bytes+30)||(bits!=24&&bits!=32)||w<1||w>1920||!h||h<-1080||h>1080)return -1;
        int hh=h<0?-h:h;row=(w*step+3)&~3u;if(offset>(u32)n||row*hh>(u32)n-offset)return -1;width=w;height=hh;topdown=h<0;bmp=1;
    }else if(!memcmp(bytes,"\x89PNG\r\n\x1a\n",8)){
        LodePNGState state;lodepng_state_init(&state);state.decoder.zlibsettings.max_output_size=9*1024*1024;unsigned w,h;
        unsigned err=lodepng_inspect(&w,&h,&state,bytes,n);if(!err&&w&&w<=1920&&h&&h<=1080)err=lodepng_decode(&rgba,&w,&h,&state,bytes,n);else err=1;lodepng_state_cleanup(&state);
        if(err)return -1;width=w;height=h;bmp=0;
    }else return -1;
    strcpy(status,"BMP / PNG | O open another image | Ctrl+F fullscreen");bds->print("IMAGE decoded: ");bds->print(path);bds->print("\n");return 0;
}
static void draw(void){
    bds->rect(0,0,800,600,0x1c2427);bds->text(16,14,path,0xf0e6c9);
    if(width&&height){int w=768,h=w*height/width;if(h>520){h=520;w=h*width/height;}int ox=(800-w)/2,oy=44+(520-h)/2;
        for(int y=0;y<h;y++)for(int x=0;x<w;x++){int sx=x*width/w,sy=y*height/h;u8*p=bmp?bytes+offset+(topdown?sy:height-1-sy)*row+sx*step:rgba+(sy*width+sx)*4;u32 c=bmp?p[2]<<16|p[1]<<8|p[0]:p[0]<<16|p[1]<<8|p[2];if(!bmp&&p[3]!=255){int a=p[3],bg=((x/8+y/8)&1)?210:170;int r=(p[0]*a+bg*(255-a))/255,g=(p[1]*a+bg*(255-a))/255,b=(p[2]*a+bg*(255-a))/255;c=r<<16|g<<8|b;}bds->rect(ox+x,oy+y,1,1,c);}}
    bds->text(16,580,status,0xe5dfc9);bds->present();
}
int app_main(BdsApi*api,int argc,char**argv){bds=api;bds->print("IMAGE viewer ready\n");char p[256];if(argc>1)open_image(argv[1]);else{if(bds->pick_file(".BMP;.PNG",p,sizeof(p)))return 0;open_image(p);}draw();
    for(;;){int k=bds->key(0);if(k==27||k==KEY_F10)break;if(k=='o'||k=='O'){if(!bds->pick_file(".BMP;.PNG",p,sizeof(p)))open_image(p);draw();}else if(k==KEY_OPEN){if(!bds->opened_path(p,sizeof(p)))open_image(p);draw();}else if(k)draw();else{bds->present();bds->sleep(40);}}
    free(bytes);free(rgba);return 0;
}
