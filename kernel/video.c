#include "os.h"
#include "font8x8.h"
#include "ui_font.h"
int screen_w=800,screen_h=600,graphics;
static u32 *fb,pitch,redpos=16,greenpos=8,bluepos=0;
/* Persistent framebuffer reservation: 80-112 MiB, clear of apps/heap/install payload. */
static u32*const canvas=(u32*)0x5000000;
int video_matches(const DisplayMode*m){return m->address==(u32)fb&&m->stride==pitch*4&&m->w==screen_w&&m->h==screen_h&&m->red==redpos&&m->green==greenpos&&m->blue==bluepos;}
void video_mode(const DisplayMode*m){fb=(u32*)m->address;pitch=m->stride/4;screen_w=m->w;screen_h=m->h;redpos=m->red;greenpos=m->green;bluepos=m->blue;graphics=1;memset(canvas,0,screen_w*screen_h*4);if(mouse_x>=screen_w)mouse_x=screen_w-1;if(mouse_y>=screen_h)mouse_y=screen_h-1;}
static char console[30][96];static int cr,cc;
static u8 custom_font[256][8];static int custom,smooth=1;static u8 ttf_atlas[128][96];static int ttf_active;
extern int truetype_load(const char*,u8[128][96]);
void font_smooth(int enabled){smooth=!!enabled;ttf_active=0;}
int font_is_smooth(void){return smooth;}
void font_reset(void){custom=smooth=ttf_active=0;}
int font_load(const char*path){
    int len=strlen(path);if(len>4&&!stricmp(path+len-4,".TTF")){static u8 next[128][96];if(truetype_load(path,next))return -1;memcpy(ttf_atlas,next,sizeof(next));ttf_active=smooth=1;return 0;}
    int f=fs_find(path),n=fs_size(f);u8 h[4],glyphs[8192];if(n<4||fs_read(f,0,h,4)!=4||h[0]!=0x36||h[1]!=4||h[3]<8||h[3]>32||(h[2]&~7)||n<4+((h[2]&1)?512:256)*h[3])return -1;
    if(fs_read(f,4,glyphs,256*h[3])!=256*h[3])return -1;
    /* Normalize fixed-width PSF1 glyphs to the desktop's 8x8 cell. */
    for(int c=0;c<256;c++){for(int j=0;j<8;j++){u8 a=glyphs[c*h[3]+j*h[3]/8],v=0;for(int i=0;i<8;i++)if(a&(128>>i))v|=1<<i;custom_font[c][j]=v;}}custom=1;smooth=ttf_active=0;return 0;
}
u8 font_row(u8 ch,int row){return custom?custom_font[ch][row]:ch<128?font8x8_basic[ch][row]:0;}
void video_init(u32 info){
    u32 total=*(u32*)info;for(u8*p=(u8*)info+8;p<(u8*)info+total;){u32 type=*(u32*)p,size=*(u32*)(p+4);if(size<8)break;if(type==8&&p[29]==1&&p[28]==32&&*(u32*)(p+12)==0){fb=(u32*)*(u32*)(p+8);pitch=*(u32*)(p+16)/4;screen_w=*(u32*)(p+20);screen_h=*(u32*)(p+24);redpos=p[32];greenpos=p[34];bluepos=p[36];if(screen_w<=3840&&screen_h<=2160&&pitch>=(u32)screen_w&&screen_w>=640&&screen_h>=480)graphics=1;}p+=(size+7)&~7u;}
}
void rect(int x,int y,int w,int h,u32 c){if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}if(x+w>screen_w)w=screen_w-x;if(y+h>screen_h)h=screen_h-y;for(int j=0;j<h;j++)for(int i=0;i<w;i++)canvas[(y+j)*screen_w+x+i]=c;}
void pixel_blend(int x,int y,u32 c){
    if(x<0||y<0||x>=screen_w||y>=screen_h)return;u32 a=c>>24;if(!a)return;
    u32*p=canvas+y*screen_w+x,b=*p;if(a==255){*p=c&0xffffff;return;}
    *p=((((c>>16)&255)*a+((b>>16)&255)*(255-a))/255<<16)|((((c>>8)&255)*a+((b>>8)&255)*(255-a))/255<<8)|(((c&255)*a+(b&255)*(255-a))/255);
}
void text_surface(u32*dest,int width,int height,int x,int y,const char*s,u32 c){
    int base=x;while(*s){u8 ch=*s++;if(ch=='\n'){x=base;y+=12;continue;}
        for(int j=0;j<(smooth?12:8);j++)for(int i=0;i<8;i++){
            if(x+i<0||x+i>=width||y+j<0||y+j>=height)continue;
            u32 a=smooth?(ch<128?(ttf_active?ttf_atlas[ch][j*8+i]:ui_font[ch][j*8+i]):0):((font_row(ch,j)&(1<<i))?255:0);
            if(!a)continue;u32*p=dest+(y+j)*width+x+i,b=*p;
            if(a==255)*p=c;else *p=((((c>>16)&255)*a+((b>>16)&255)*(255-a))/255<<16)|((((c>>8)&255)*a+((b>>8)&255)*(255-a))/255<<8)|(((c&255)*a+(b&255)*(255-a))/255);
        }x+=8;
    }
}
void text(int x,int y,const char*s,u32 c){text_surface(canvas,screen_w,screen_h,x,y,s,c);}
void present_region(int ox,int oy,int w,int h){
    if(!graphics||ox<0||oy<0||w<1||h<1||ox>screen_w-w||oy>screen_h-h)return;
    if(redpos==16&&greenpos==8&&bluepos==0){
        for(int y=oy;y<oy+h;y++){const u32*s=canvas+y*screen_w+ox;u32*d=fb+y*pitch+ox;u32 n=w;__asm__ volatile("cld; rep movsl":"+D"(d),"+S"(s),"+c"(n)::"memory");}
    }else for(int y=oy;y<oy+h;y++)for(int x=ox;x<ox+w;x++){u32 c=canvas[y*screen_w+x];fb[y*pitch+x]=((c>>16)&255)<<redpos|((c>>8)&255)<<greenpos|(c&255)<<bluepos;}
}
void present(void){present_region(0,0,screen_w,screen_h);}
void video_scaled(const u32*src,int sw,int sh,int ox,int oy,int w,int h){
    if(w<1||h<1||w>3840||ox<0||oy<0||ox+w>screen_w||oy+h>screen_h)return;static int sx[3840];for(int x=0;x<w;x++)sx[x]=x*sw/w;
    for(int y=0;y<h;y++){const u32*row=src+y*sh/h*sw;u32*dest=canvas+(oy+y)*screen_w+ox;for(int x=0;x<w;x++)dest[x]=row[sx[x]];}
}
/* Fill the desktop by cropping a uniformly scaled image, never stretching it. */
void video_stretch(const u32*src,int w,int h){
    int sw=w,sh=h;if(screen_w*h>screen_h*w)sh=w*screen_h/screen_w;else sw=h*screen_w/screen_h;
    int ox=(w-sw)/2,oy=(h-sh)/2;
    if(!smooth){for(int y=0;y<screen_h;y++)for(int x=0;x<screen_w;x++)canvas[y*screen_w+x]=src[(oy+y*sh/screen_h)*w+ox+x*sw/screen_w];return;}
    static int ix[3840],ax[3840];for(int x=0;x<screen_w;x++){int p=screen_w>1?x*(sw-1)*256/(screen_w-1):0;ix[x]=ox+(p>>8);ax[x]=p&255;}
    for(int y=0;y<screen_h;y++){int p=screen_h>1?y*(sh-1)*256/(screen_h-1):0,sy=oy+(p>>8),by=p&255,ny=sy+1<h?sy+1:sy;
        for(int x=0;x<screen_w;x++){int sx=ix[x],nx=sx+1<w?sx+1:sx,bx=ax[x];u32 a=src[sy*w+sx],b=src[sy*w+nx],c=src[ny*w+sx],d=src[ny*w+nx],v=0;
            for(int shift=0;shift<=16;shift+=8){u32 upper=((a>>shift)&255)*(256-bx)+((b>>shift)&255)*bx,lower=((c>>shift)&255)*(256-bx)+((d>>shift)&255)*bx;v|=((upper*(256-by)+lower*by)>>16)<<shift;}canvas[y*screen_w+x]=v;}
    }
}
void blit(const u32*src,int w,int h){if(!graphics)return;int scale=screen_w/w;if(screen_h/h<scale)scale=screen_h/h;if(scale<1)scale=1;int ox=(screen_w-w*scale)/2,oy=(screen_h-h*scale)/2;rect(0,0,screen_w,screen_h,0);for(int y=0;y<h;y++)for(int x=0;x<w;x++)rect(ox+x*scale,oy+y*scale,scale,scale,src[y*w+x]);present();}
void console_draw(int x,int y,int rows,int columns){int start=cr>=rows?cr-rows+1:0;for(int j=0;j<rows&&start+j<30;j++){char line[96];memcpy(line,console[start+j],96);if(columns<95)line[columns]=0;text(x,y+j*14,line,0x84e79f);}}
void clear_console(void){memset(console,0,sizeof(console));cr=cc=0;}
void print(const char*s){serial(s);while(*s){char c=*s++;if(c=='\r')continue;if(c=='\n'){cr++;cc=0;}else if(c==8){if(cc)console[cr][--cc]=0;}else if(c>=32){console[cr][cc++]=c;console[cr][cc]=0;if(cc>=94){cc=0;cr++;}}if(cr>=30){memcpy(console,console[1],29*96);memset(console[29],0,96);cr=29;}}}
void console_full(void){if(graphics){rect(0,0,screen_w,screen_h,0x171d1c);console_draw(12,20,30,94);text(12,screen_h-22,"F1 desktop | BDX-OS terminal",0xd4b46b);present();}else{volatile u16*v=(u16*)0xb8000;int start=cr>=25?cr-24:0;for(int y=0;y<25;y++)for(int x=0;x<80;x++)v[y*80+x]=0x0a00|(console[start+y][x]?console[start+y][x]:' ');}}
