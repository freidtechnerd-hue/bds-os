/* minimp3's CC0 implementation compiled with x87, without SIMD. */
#include "bds.h"
#define MINIMP3_NO_SIMD
#define MINIMP3_IMPLEMENTATION
#include "../third_party/minimp3/minimp3.h"
static mp3dec_t decoder;static mp3d_sample_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
static const u8*data;static u32 length,pos,phase;static int samples,index,channels,hz;
static int next_frame(void){
    while(pos<length){mp3dec_frame_info_t info;int n=mp3dec_decode_frame(&decoder,data+pos,length-pos,pcm,&info);if(info.frame_bytes<=0){pos=length;return -1;}pos+=info.frame_bytes;if(n&&info.hz>=8000&&info.hz<=48000&&(info.channels==1||info.channels==2)){samples=n;index=0;channels=info.channels;hz=info.hz;return 0;}}return -1;
}
int mp3_start(const u8*bytes,u32 size){data=bytes;length=size;pos=phase=0;samples=index=0;mp3dec_init(&decoder);return next_frame();}
int mp3_sample(short*out){if(index>=samples&&next_frame())return -1;out[0]=pcm[index*channels];out[1]=pcm[index*channels+(channels==2)];phase+=hz;while(phase>=48000){phase-=48000;index++;}return 0;}
