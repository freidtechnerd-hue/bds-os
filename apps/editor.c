#include "common.h"
int app_main(BdsApi*a,int argc,char**argv){
    char filename[256];const char*source=argc>1?argv[1]:"/NOTES.TXT";int pl=0;while(source[pl]&&pl<255){filename[pl]=source[pl];pl++;}filename[pl]=0;const char*path=filename;char*buf=a->alloc(16384);if(!buf)return 1;int f=a->find(path),n=a->size(f),cursor,dirty=0,selected=0;const char*status="F2 save | Ctrl+A/C/X/V | arrows move | ESC exit";
    if(n>16382){a->print("EDITOR: file exceeds 16KB limit\n");return 1;}if(n<0)n=0;else if(a->read(f,0,buf,n)!=n)return 1;buf[n]=0;cursor=n;a->print("EDITOR ready\n");
    for(;;){a->rect(0,0,a->width,a->height,0x18251e);a->rect(0,0,a->width,34,0xbfc5b7);a->text(14,12,path,0x18251e);a->text(14,a->height-24,status,0xd8c582);
        int row=0,col=0,cols=(a->width-34)/8,rows=(a->height-100)/12;
        for(int i=0;i<cursor;i++){if(buf[i]=='\n'||++col>=cols){row++;col=0;}}
        int first=row>=rows?row-rows+1:0;row=col=0;
        for(int i=0;i<=n;i++){int x=14+col*8,y=50+(row-first)*12;if(row>=first&&row<first+rows){if(selected&&i<n)a->rect(x,y,8,12,0x36594b);if(i==cursor)a->rect(x,y+10,7,2,0xffd782);if(i<n&&buf[i]!='\n'){char s[2]={buf[i],0};a->text(x,y,s,0x8ae5a4);}}if(i==n)break;if(buf[i]=='\n'||++col>=cols){row++;col=0;}}
        a->present();
        int k=a->key(0);if(!k){a->sleep(20);continue;}if(k==27){if(dirty){status="Unsaved changes: F2 save or Q discard";for(;;){k=a->key(0);if(k=='q'||k=='Q')return 0;if(k==KEY_F1+1){if(!a->write(path,buf,n))return 0;status="Save failed. ESC returns.";break;}if(k==27)break;a->sleep(20);}}else return 0;}
        else if(k==KEY_OPEN&&a->version>=2){char p[256];if(!a->opened_path(p,sizeof(p))){if(dirty){status="Save your changes before opening another file";continue;}int id=a->find(p),size=a->size(id);if(size<0||size>16382){status="Cannot open file: missing or exceeds 16KB";continue;}if(a->read(id,0,buf,size)!=size){status="File read failed";continue;}int j=0;while((filename[j]=p[j]))j++;n=cursor=size;buf[n]=0;status="File opened | F2 save | ESC exit";}}
        else if(k==KEY_F1+1){if(a->write(path,buf,n))status="Save failed: disk full or invalid path";else{dirty=0;status="Saved. ESC exits.";a->print("EDITOR saved\n");}}
        else if(k==KEY_SELECT_ALL){selected=1;status="All text selected | Ctrl+C copy | Ctrl+X cut";}
        else if((k==KEY_COPY||k==KEY_CUT)&&a->version>=7){if(!selected)status="Press Ctrl+A to select the document first";else if(a->clipboard_set(buf,n))status="Clipboard limit: 4095 bytes; document was not changed";else{status="Copied to shared clipboard";if(k==KEY_CUT){n=cursor=selected=0;buf[0]=0;dirty=1;status="Cut to shared clipboard";}}}
        else if(k==KEY_PASTE&&a->version>=7){char clip[4096];int size=a->clipboard_get(clip,sizeof(clip));if(size<0)status="Clipboard unavailable";else if(size+(selected?0:n)>16382)status="Paste exceeds 16KB editor limit";else if(size){if(selected){n=cursor=selected=0;}for(int i=n;i>=cursor;i--)buf[i+size]=buf[i];for(int i=0;i<size;i++)buf[cursor+i]=clip[i];cursor+=size;n+=size;dirty=1;status="Pasted from shared clipboard";a->print("EDITOR pasted\n");}}
        else if(k==KEY_LEFT){if(cursor)cursor--;selected=0;}else if(k==KEY_RIGHT){if(cursor<n)cursor++;selected=0;}
        else if(k==KEY_UP||k==KEY_DOWN){cursor+=k==KEY_UP?-cols:cols;if(cursor<0)cursor=0;if(cursor>n)cursor=n;selected=0;}
        else if(k==8&&selected){n=cursor=selected=0;buf[0]=0;dirty=1;}
        else if(k==8&&cursor){for(int i=cursor-1;i<n;i++)buf[i]=buf[i+1];cursor--;n--;dirty=1;}
        else if((k==13||(k>=32&&k<127))&&(selected||n<16382)){if(selected){n=cursor=selected=0;buf[0]=0;}for(int i=n;i>=cursor;i--)buf[i+1]=buf[i];buf[cursor++]=k==13?'\n':k;n++;dirty=1;}
    }
}
