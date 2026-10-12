#include "common.h"
int app_main(BdsApi*a,int argc,char**argv){
    char filename[256];const char*source=argc>1?argv[1]:"/NOTES.TXT";int pl=0;while(source[pl]&&pl<255){filename[pl]=source[pl];pl++;}filename[pl]=0;const char*path=filename;char*buf=a->alloc(16384);if(!buf)return 1;int f=a->find(path),n=a->size(f),cursor,dirty=0;const char*status="F2 save | ESC exit | arrows move cursor";
    if(n>16382){a->print("EDITOR: file exceeds 16KB limit\n");return 1;}if(n<0)n=0;else if(a->read(f,0,buf,n)!=n)return 1;buf[n]=0;cursor=n;a->print("EDITOR ready\n");
    for(;;){a->rect(0,0,a->width,a->height,0x18251e);a->rect(0,0,a->width,34,0xbfc5b7);a->text(14,12,path,0x18251e);a->text(14,a->height-24,status,0xd8c582);int x=14,y=50;
        for(int i=0;i<=n;i++){if(i==cursor)a->rect(x,y+8,7,2,0xffd782);if(i==n)break;char s[2]={buf[i],0};if(buf[i]=='\n'){x=14;y+=12;}else{if(y<a->height-40)a->text(x,y,s,0x8ae5a4);x+=8;if(x>a->width-20){x=14;y+=12;}}}a->present();
        int k=a->key(0);if(!k){a->sleep(20);continue;}if(k==27){if(dirty){status="Unsaved changes: F2 save or Q discard";for(;;){k=a->key(0);if(k=='q'||k=='Q')return 0;if(k==KEY_F1+1){if(!a->write(path,buf,n))return 0;status="Save failed. ESC returns.";break;}if(k==27)break;a->sleep(20);}}else return 0;}
        else if(k==KEY_OPEN&&a->version>=2){char p[256];if(!a->opened_path(p,sizeof(p))){if(dirty){status="Save your changes before opening another file";continue;}int id=a->find(p),size=a->size(id);if(size<0||size>16382){status="Cannot open file: missing or exceeds 16KB";continue;}if(a->read(id,0,buf,size)!=size){status="File read failed";continue;}int j=0;while((filename[j]=p[j]))j++;n=cursor=size;buf[n]=0;status="File opened | F2 save | ESC exit";}}
        else if(k==KEY_F1+1){if(a->write(path,buf,n))status="Save failed: disk full or invalid path";else{dirty=0;status="Saved. ESC exits.";a->print("EDITOR saved\n");}}
        else if(k==KEY_LEFT&&cursor)cursor--;else if(k==KEY_RIGHT&&cursor<n)cursor++;
        else if(k==8&&cursor){for(int i=cursor-1;i<n;i++)buf[i]=buf[i+1];cursor--;n--;dirty=1;}
        else if((k==13||(k>=32&&k<127))&&n<16382){for(int i=n;i>=cursor;i--)buf[i+1]=buf[i];buf[cursor++]=k==13?'\n':k;n++;dirty=1;}
    }
}
