#include "bds.h"
/* Verify actual guest bulk reads, unaligned boundaries and EOF clamping. */
static unsigned char data[98317];
int app_main(BdsApi*a,int argc,char**argv){
    int f=a->find("/READTEST.BIN");const unsigned offsets[]={0,1,511,512,513,32767,32768,65531,98310,98317,98318};
    const unsigned counts[]={1,513,32768,65536,98317};
    if(a->size(f)!=98317)return 1;
    for(unsigned i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++)for(unsigned j=0;j<sizeof(counts)/sizeof(counts[0]);j++){
        unsigned off=offsets[i],n=counts[j],expected=off>=98317?0:98317-off;if(expected>n)expected=n;
        int read=a->read(f,off,data,n);if(read!=(int)expected)return 2;
        for(unsigned k=0;k<expected;k++)if(data[k]!=(unsigned char)(((off+k)*37+((off+k)>>8))&255))return 3;
    }
    a->print("READ PROBE: bulk, unaligned and EOF bytes verified\n");return 0;
}
