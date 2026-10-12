#include "common.h"
int app_main(BdsApi*a,int argc,char**argv){
    if(a->version<2||!a->audio_available()){a->print("PLAYER: supported AC'97 or HDA audio hardware required\n");return 1;}
    char path[256];if(argc>1){int n=0;while(argv[1][n]&&n<255){path[n]=argv[1][n];n++;}path[n]=0;}else if(a->pick_file(".MP3;.MID;.MIDI",path,sizeof(path)))return 0;
    if(a->music(path,1)){a->print("PLAYER: unsupported or corrupt music file\n");return 1;}a->print("PLAYER started\n");
    for(;;){a->rect(0,0,a->width,a->height,0x17251d);a->text(32,40,"BDS AUDIO PLAYER",0xe8d49a);a->text(32,80,path,0xe8d49a);a->text(32,120,"MP3 decoding / MIDI software instruments",0x95bda4);a->text(32,150,"ESC stop | Options: audio, font and fullscreen",0x95bda4);a->present();int k=a->key(0);if(k==27)break;if(k==KEY_OPEN&&!a->opened_path(path,sizeof(path))&&a->music(path,1))a->print("PLAYER: file failed to load\n");a->sleep(20);}a->audio_stop();return 0;
}
