#include "bds.h"
static void num(BdsApi*a,int n){char s[16];int i=0;do{s[i++]='0'+n%10;n/=10;}while(n);char out[16];int j=0;while(i)out[j++]=s[--i];out[j]=0;a->print(out);}
int app_main(BdsApi*a,int argc,char**argv){
    a->print("DISPLAY PROBE ready\n");int prev=0;
    for(;;){a->rect(0,0,800,600,0x112233);a->rect(350,250,100,100,0xff0000);a->present();int x,y,down=a->pointer(&x,&y)&1;
        if(down&&!prev){a->print("POINTER ");num(a,x);a->print(",");num(a,y);a->print("\n");}prev=down;
        int k=a->key(0);if(k==27)return 0;if(k=='v'){int w,h;a->viewport(&w,&h);a->print("VIEWPORT ");num(a,w);a->print("x");num(a,h);a->print("\n");}a->sleep(20);
    }
}
