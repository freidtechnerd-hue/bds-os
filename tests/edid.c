#include <assert.h>
#include <string.h>
#include "../kernel/edid.h"
static void checksum(unsigned char*b){unsigned n=0;for(int i=0;i<127;i++)n+=b[i];b[127]=-n;}
int main(void){
    unsigned char b[128]={0};int w=0,h=0;
    b[0]=0;memset(b+1,255,6);b[18]=1;b[19]=4;b[24]=2;
    b[54]=1;b[56]=0;b[58]=0x50;b[59]=0xd0;b[61]=0x20;checksum(b);
    assert(!edid_preferred(b,&w,&h)&&w==1280&&h==720);
    b[127]^=1;assert(edid_preferred(b,&w,&h));b[127]^=1;
    b[71]=128;checksum(b);assert(edid_preferred(b,&w,&h));
    memset(b,0,sizeof(b));b[0]=0x70;b[1]=0x13;b[2]=23;b[3]=3;b[5]=3;b[7]=20;
    b[11]=0x88;b[12]=0xff;b[13]=14;b[20]=0x6f;b[21]=8;
    unsigned sum=0;for(int i=1;i<28;i++)sum+=b[i];b[28]=-sum;checksum(b);
    assert(!edid_extension(b,&w,&h)&&w==3840&&h==2160);
    b[28]^=1;checksum(b);assert(edid_extension(b,&w,&h));
    b[2]=255;checksum(b);assert(edid_extension(b,&w,&h));
    memset(b,0,sizeof(b));b[0]=2;b[1]=3;b[2]=126;b[3]=15;checksum(b);
    assert(edid_extension(b,&w,&h));return 0;
}
