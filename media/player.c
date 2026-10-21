/* Streaming native media player with DVD VM navigation; no CSS decryption. */
#include "libc.h"
#include "bds.h"
#include "mpeg2.h"
#include "a52.h"
#include "pl_mpeg.h"
#include "dvdnav/dvdnav.h"
BdsApi*bds;
static dvdnav_t*dvd;static u8 dvd_block[2048];static int dvd_pos,dvd_len,dvd_menu,dvd_jump;static u32 dvd_still,dvd_pts_end;
static int movie_x,movie_y,movie_w,movie_h;static u32 dvd_w=720,dvd_h=480;
static int dvd_read(void*,u32);static int dvd_open(const char*);static int dvd_detect(void);static int dvd_key(int);static void dvd_mouse(int,int,int);static void dvd_highlight(void);static void dvd_reset(void);

enum {IDLE,MUSIC,WAVE,VIDEO,CD};
static int mode,paused,repeat,volume=96,quit,request,prev_buttons,failure;
static char path[256],message[128]="Open a file, DVD title, or audio CD.",codec[80];
static u32 position,duration,anchor,pause_tick,last_ui,frames;
static u32 pixels[400*300];static int picture_w,picture_h,dar_n=4,dar_d=3;
static char list[48][256];static int list_count,list_selected,list_open;
static int fd=-1;static u32 file_size,file_pos,cache_pos,cache_size;static u8 cache[32768],packet_data[65536];
static u32 wav_start,wav_bytes,wav_rate;static int wav_channels,wav_bits,wav_align;
static mpeg2dec_t*video;static a52_state_t*ac3;static plm_buffer_t*mp2_buffer;static plm_audio_t*mp2;
static u8 ac3_data[8192];static u32 ac3_size;static short pcm[8192];
static int audio_id=-1,video_id=-1,have_pts;static u32 first_pts,frame_ms=40;
static CdInfo toc;static int cd_device=-1,cd_image=-1,track;static u32 cd_lba,cd_end;
static u32 le32(const u8*p){return p[0]|(u32)p[1]<<8|(u32)p[2]<<16|(u32)p[3]<<24;}
static u32 be16(const u8*p){return (u32)p[0]<<8|p[1];}
static int suffix(const char*p,const char*e){int n=strlen(p),m=strlen(e);return n>=m&&!strcasecmp(p+n-m,e);}
static int supported(const char*p){return suffix(p,".MP3")||suffix(p,".MID")||suffix(p,".MIDI")||suffix(p,".WAV")||suffix(p,".MPG")||suffix(p,".MPEG")||suffix(p,".M2V")||suffix(p,".VOB")||suffix(p,".CUE")||suffix(p,".IFO");}
static void status(const char*s){strncpy(message,s,sizeof(message)-1);message[sizeof(message)-1]=0;bds->print("PLAYER: ");bds->print(message);bds->print("\n");}
static void fail(const char*s){failure=1;status(s);}
static void button(int x,int y,int w,const char*s,int active){bds->rect(x,y,w,28,active?0x3b766b:0x344753);bds->text(x+8,y+10,s,0xffffff);}
static void ui(void){
    bds->rect(0,0,800,600,0x14212b);button(16,12,80,"Open",0);button(104,12,80,"DVD",0);button(192,12,80,"CD",0);button(280,12,112,"Playlist",list_open);
    bds->text(410,22,"BDS MEDIA PLAYER",0xe3c184);char line[100];snprintf(line,sizeof(line),"%.91s",path);bds->text(24,52,line,0xbad2da);
    if(picture_w&&picture_h){int vw,vh;bds->viewport(&vw,&vh);int w=752,h=(uint64_t)w*dar_d*vw*600/((uint64_t)dar_n*vh*800);if(h>400){h=400;w=(uint64_t)h*dar_n*vh*800/((uint64_t)dar_d*vw*600);}if(w<1)w=1;if(h<1)h=1;movie_x=(800-w)/2;movie_y=80+(400-h)/2;movie_w=w;movie_h=h;bds->surface_blit(pixels,picture_w,picture_h,movie_x,movie_y,w,h);dvd_highlight();}
    else{bds->text(64,190,mode==CD?"AUDIO CD":mode==MUSIC||mode==WAVE?"AUDIO PLAYBACK":"VIDEO / MUSIC / DVD / AUDIO CD",0xe3c184);bds->text(64,224,codec,0xbad2da);bds->text(64,266,"Space play/pause | Left/Right seek | +/- volume",0xbad2da);bds->text(64,292,"O open | D DVD | C CD | N/P next/previous | L repeat",0xbad2da);bds->text(64,318,"Ctrl+F fullscreen | Escape/F10 exit",0xbad2da);}
    bds->rect(24,488,752,10,0x344753);if(duration){u32 w=(u32)((uint64_t)position*752/duration);if(w>752)w=752;bds->rect(24,488,w,10,0x57af99);}
    button(16,512,108,(!mode||paused)?"Play":"Pause",mode&&!paused);button(132,512,80,"Stop",0);button(220,512,80,"Prev",0);button(308,512,80,"Next",0);button(396,512,100,"Repeat",repeat);button(552,512,44,"-",0);button(660,512,44,"+",0);
    snprintf(line,sizeof(line),"%u%%",volume*100/127);bds->text(608,522,line,0xffffff);
    snprintf(line,sizeof(line),"%u:%02u / %u:%02u   %s",position/60000,position/1000%60,duration/60000,duration/1000%60,paused?"Paused":mode?"Playing":"Stopped");bds->text(24,552,line,0xbad2da);bds->text(24,576,message,0xe3c184);
    if(list_open){bds->rect(38,76,724,400,0x263a46);bds->text(52,88,"Select: arrows/Enter or click; Escape closes list",0xffffff);int first=list_selected>=14?list_selected-13:0;for(int i=first;i<list_count&&i<first+14;i++){int y=114+(i-first)*24;if(i==list_selected)bds->rect(48,y-4,704,24,0x3b766b);snprintf(line,sizeof(line),"%.84s",list[i]);bds->text(56,y+4,line,0xffffff);}if(!list_count)bds->text(56,138,"No compatible files found.",0xe3c184);}
    bds->present();last_ui=bds->ticks();
}
static void pause_toggle(void){if(!mode){if(path[0])request=cd_device>=0?12:1;return;}paused=!paused;if(paused)pause_tick=bds->ticks();else anchor+=bds->ticks()-pause_tick;bds->audio_pause(paused);bds->music_control(1,paused);status(paused?"Paused":"Playing");}
static int poll(void){
    int k;while((k=bds->key(0))){if(list_open){if(k==27)list_open=0;else if(k==KEY_UP&&list_selected)list_selected--;else if(k==KEY_DOWN&&list_selected+1<list_count)list_selected++;else if(k==13&&list_count){strcpy(path,list[list_selected]);request=1;list_open=0;}continue;}
        if(dvd_key(k))continue;
        if(k==27||k==KEY_F10){quit=1;return 1;}if(k==KEY_OPEN){if(!bds->opened_path(path,sizeof(path)))request=1;}
        if(k==' ')pause_toggle();if(k=='o'||k=='O')request=2;if(k=='d'||k=='D')request=3;if(k=='c'||k=='C')request=4;if(k=='s'||k=='S')request=5;if(k=='n'||k=='N')request=6;if(k=='p'||k=='P')request=7;
        if(k=='l'||k=='L')repeat=!repeat;if(k=='+'||k=='=')volume+=8;if(k=='-')volume-=8;if(k==KEY_RIGHT)request=8;if(k==KEY_LEFT)request=9;
    }
    int x,y,b=bds->pointer(&x,&y),click=(b&1)&&!(prev_buttons&1);prev_buttons=b;if(dvd&&!list_open)dvd_mouse(x,y,click);
    if(click&&x>=0&&y>=0){if(list_open&&y>=110&&y<446&&x>=48&&x<752){int first=list_selected>=14?list_selected-13:0;int i=first+(y-110)/24;if(i<list_count){list_selected=i;strcpy(path,list[i]);request=1;list_open=0;}}
        else if(y>=12&&y<40){if(x>=16&&x<96)request=2;else if(x>=104&&x<184)request=3;else if(x>=192&&x<272)request=4;else if(x>=280&&x<392)request=10;}
        else if(y>=512&&y<540){if(x>=16&&x<124)pause_toggle();else if(x>=132&&x<212)request=5;else if(x>=220&&x<300)request=7;else if(x>=308&&x<388)request=6;else if(x>=396&&x<496)repeat=!repeat;else if(x>=552&&x<596)volume-=8;else if(x>=660&&x<704)volume+=8;}}
    if(volume<0)volume=0;if(volume>127)volume=127;bds->music_volume(volume);
    if(bds->ticks()-last_ui>=100)ui();return request||quit;
}
static int ready(void){do{if(poll())return 0;if(paused)bds->sleep(10);}while(paused);return 1;}
static int audio(const short*p,u32 n,u32 rate,int channels){
    if(!bds->audio_available())return ready();u32 at=0;short scaled[2304];
    while(at<n){if(!ready())return 0;u32 take=n-at;if(take>1152)take=1152;for(u32 i=0;i<take*channels;i++)scaled[i]=p[at*channels+i]*volume/127;int wrote=bds->audio_write(scaled,take,rate,channels);if(wrote<0){fail("Audio device rejected PCM data.");return 0;}at+=wrote;if(!wrote)bds->sleep(2);}
    return 1;
}
static void seek_file(u32 at){file_pos=at>file_size?file_size:at;cache_size=0;}
static int read_file(void*out,u32 n){if(dvd)return dvd_read(out,n);u8*p=out;u32 got=0;while(got<n&&file_pos<file_size){if(!cache_size||file_pos<cache_pos||file_pos>=cache_pos+cache_size){cache_pos=file_pos;int r=bds->read(fd,file_pos,cache,sizeof(cache));if(r<=0)return got?got:-1;cache_size=r;}u32 part=cache_pos+cache_size-file_pos;if(part>n-got)part=n-got;if(p)memcpy(p+got,cache+file_pos-cache_pos,part);file_pos+=part;got+=part;}return got;}
static int byte(void){u8 b;return read_file(&b,1)==1?b:-1;}
static void close_media(void){if(dvd){dvdnav_close(dvd);dvd=0;}dvd_menu=dvd_pos=dvd_len=0;bds->audio_pause(0);bds->music_control(1,0);bds->music_data(0,0,0);bds->audio_stop();if(video)mpeg2_close(video);if(ac3)a52_free(ac3);if(mp2)plm_audio_destroy(mp2);video=0;ac3=0;mp2=0;mp2_buffer=0;mode=IDLE;paused=0;fd=-1;ac3_size=0;audio_id=video_id=-1;have_pts=0;frames=position=duration=0;}
static int open_file(const char*p){fd=bds->find(p);int size=bds->size(fd);if(size<1){status("File could not be opened.");return 0;}file_size=size;seek_file(0);return 1;}
static void dirname(char*out,const char*in){strcpy(out,in);char*p=strrchr(out,'/');if(p)p[1]=0;else out[0]=0;}
static void playlist(int dvd){
    if(mode&&!paused)pause_toggle();list_count=list_selected=0;char dir[256];FsEntry e;
    if(dvd){status("Scanning mounted DVD volumes...");ui();for(int v=0;v<32&&list_count<48;v++){snprintf(dir,sizeof(dir),"V%d:/VIDEO_TS",v);for(int i=0;i<128&&list_count<48;i++){if(bds->read_dir(dir,i,&e)<=0)break;if(e.type==1&&!strncasecmp(e.name,"VTS_",4)&&suffix(e.name,"_1.VOB"))snprintf(list[list_count++],256,"%s/%s",dir,e.name);}}status(list_count?"Choose a DVD title. Menus/encrypted discs are unsupported.":"No DVD titles found. Use Open for a VOB or MPEG file.");}
    else{dirname(dir,path);if(!dir[0])strcpy(dir,"/");for(int i=0;i<128&&list_count<48;i++){if(bds->read_dir(dir,i,&e)<=0)break;if(e.type==1&&supported(e.name))snprintf(list[list_count++],256,"%s%s",dir,e.name);}status("Playlist: files in the current folder.");}
    list_open=1;
}
static int cd_track(int next){
    while(next>=0&&next<(int)toc.count&&(toc.tracks[next].control&4))next+=next>=track?1:-1;
    if(next<0||next>=(int)toc.count)return 0;track=next;cd_lba=toc.tracks[track].lba;cd_end=track+1<(int)toc.count?toc.tracks[track+1].lba:toc.leadout;
    if(cd_end<=cd_lba)return 0;mode=CD;position=0;duration=(cd_end-cd_lba)*1000/75;bds->audio_stop();snprintf(codec,sizeof(codec),"Track %u | CD-DA 44.1 kHz stereo",toc.tracks[track].number);status(codec);return 1;
}
static int open_cd(void){close_media();cd_device=cd_image=-1;int n=bds->cd_count();for(int i=0;i<n;i++)if(!bds->cd_info(i,&toc)){track=0;for(u32 t=0;t<toc.count;t++)if(!(toc.tracks[t].control&4)){cd_device=i;strcpy(path,toc.model);if(!bds->audio_available()){status("No supported audio device. Configure AC97 or Intel HDA.");return 0;}return cd_track(t);}}status(n?"No readable audio CD. Data discs do not contain CD-DA tracks.":"No legacy IDE audio-CD drive. Open a BIN/CUE image instead.");return 0;}
static int cue(void){
    if(file_size>16384)return 0;char text[16385],name[256],folder[256];if(read_file(text,file_size)!=(int)file_size)return 0;text[file_size]=0;char*f=strstr(text,"FILE ");if(!f||!(f=strchr(f,'\"')))return 0;char*end=strchr(++f,'\"');if(!end||end-f>220)return 0;memcpy(name,f,end-f);name[end-f]=0;if(!strstr(end,"BINARY"))return 0;
    dirname(folder,path);char bin[256];if(strlen(folder)+strlen(name)>=sizeof(bin))return 0;strcpy(bin,folder);strcat(bin,name);cd_image=bds->find(bin);int size=bds->size(cd_image);if(size<2352||size%2352)return 0;memset(&toc,0,sizeof(toc));toc.leadout=size/2352;cd_device=-1;
    char*p=text;while((p=strstr(p,"TRACK "))&&toc.count<99){int no=atoi(p+6);if(no<1||no>99)return 0;char*next=strstr(p+6,"TRACK ");char*idx=strstr(p,"INDEX 01 ");if(!idx||(next&&idx>next))return 0;int m=0,s=0,fr=0;if(sscanf(idx+9,"%d:%d:%d",&m,&s,&fr)!=3||m<0||s<0||s>=60||fr<0||fr>=75)return 0;u32 lba=(m*60+s)*75+fr;if(lba>=toc.leadout||(toc.count&&lba<=toc.tracks[toc.count-1].lba))return 0;char*audio_type=strstr(p,"AUDIO");int is_audio=audio_type&&audio_type<idx;toc.tracks[toc.count++]=(CdTrack){lba,no,is_audio?0:4,0};p=idx+9;}
    track=0;return toc.count&&cd_track(0);
}
static int start(void){
    close_media();failure=0;cd_device=cd_image=-1;picture_w=picture_h=0;codec[0]=0;anchor=bds->ticks();if(suffix(path,".IFO"))return dvd_open(path);if(!open_file(path))return 0;
    if(suffix(path,".MP3")||suffix(path,".MID")||suffix(path,".MIDI")){if(bds->music(path,repeat)){status("Unsupported/corrupt music, over 16 MiB, or no audio device.");return 0;}mode=MUSIC;strcpy(codec,suffix(path,".MP3")?"MPEG Layer III audio":"MIDI software instruments");}
    else if(suffix(path,".CUE")){if(!cue()){status("Unsupported CUE. Use one BINARY file with INDEX 01 tracks.");return 0;}}
    else if(suffix(path,".WAV")){u8 h[16];if(read_file(h,12)!=12||memcmp(h,"RIFF",4)||memcmp(h+8,"WAVE",4))goto bad;int fmt=0;while(file_pos+8<=file_size){if(read_file(h,8)!=8)goto bad;u32 n=le32(h+4),at=file_pos;if(n>file_size-at)goto bad;if(!memcmp(h,"fmt ",4)){if(n<16||read_file(h,16)!=16)goto bad;wav_channels=h[2]|h[3]<<8;wav_rate=le32(h+4);wav_align=h[12]|h[13]<<8;wav_bits=h[14]|h[15]<<8;if(h[0]!=1||h[1]||(wav_channels!=1&&wav_channels!=2)||(wav_bits!=8&&wav_bits!=16)||wav_rate<8000||wav_rate>96000||wav_align!=wav_channels*wav_bits/8)goto bad;fmt=1;}else if(!memcmp(h,"data",4)&&fmt){wav_start=at;wav_bytes=n;mode=WAVE;duration=(uint64_t)n*1000/(wav_rate*wav_align);snprintf(codec,sizeof(codec),"PCM WAV | %u Hz | %d-bit | %d channels",wav_rate,wav_bits,wav_channels);break;}seek_file(at+n+(n&1));}if(mode!=WAVE)goto bad;}
    else{video=mpeg2_init();ac3=a52_init(0);mp2_buffer=plm_buffer_create_with_capacity(32768);mp2=plm_audio_create_with_buffer(mp2_buffer,1);if(!video||!ac3||!mp2)goto bad;mpeg2_accel(0);mode=VIDEO;strcpy(codec,"MPEG-1/2 video with MP2, AC-3 or 16-bit DVD PCM");}
    if(mode!=VIDEO&&!bds->audio_available()){close_media();status("No supported audio device. Configure AC97 or Intel HDA.");return 0;}bds->print("PLAYER started\n");status(codec);return 1;
bad:close_media();status("Unsupported or damaged media file.");return 0;
}
static int video_bytes(u8*p,int n,u32 pts,int pts_valid){
    if(pts_valid)mpeg2_tag_picture(video,pts,1);mpeg2_buffer(video,p,p+n);const mpeg2_info_t*info=mpeg2_info(video);
    for(int limit=0;limit<4096;limit++){mpeg2_state_t st=mpeg2_parse(video);if(st==STATE_BUFFER)return 1;if(!ready())return 0;
        if(st==STATE_SEQUENCE||st==STATE_SEQUENCE_MODIFIED){const mpeg2_sequence_t*s=info->sequence;if(!s||!s->width||!s->height||s->width>1024||s->height>768||s->chroma_width!=s->width/2||s->chroma_height!=s->height/2){fail("Video exceeds 1024x768 or uses unsupported chroma.");return 0;}dar_n=s->display_width*(s->pixel_width?s->pixel_width:1);dar_d=s->display_height*(s->pixel_height?s->pixel_height:1);if(dar_n<1||dar_d<1){dar_n=4;dar_d=3;}/* Reduce before UI products. */while(dar_n>4096||dar_d>4096){dar_n=(dar_n+1)/2;dar_d=(dar_d+1)/2;}frame_ms=s->frame_period/27000;if(!frame_ms)frame_ms=40;duration=s->byte_rate?file_size/(s->byte_rate/1000+1):0;}
        if((st==STATE_SLICE||st==STATE_END||st==STATE_INVALID_END)&&info->display_fbuf&&info->sequence){const mpeg2_sequence_t*s=info->sequence;const mpeg2_picture_t*pic=info->display_picture;u32 target=frames*frame_ms;
            if(pic&&(pic->flags&PIC_FLAG_TAGS)){u32 t=pic->tag/90;if(!have_pts){first_pts=t;anchor=bds->ticks();have_pts=1;}target=t>=first_pts?t-first_pts:0;}
            while((int)(anchor+target-bds->ticks())>0){if(!ready())return 0;bds->sleep(2);}position=target;picture_w=400;picture_h=300;
            const u8*ybuf=info->display_fbuf->buf[0],*ubuf=info->display_fbuf->buf[1],*vbuf=info->display_fbuf->buf[2];
            for(int y=0;y<300;y++){int sy=y*s->picture_height/300;for(int x=0;x<400;x++){int sx=x*s->picture_width/400;int yy=ybuf[sy*s->width+sx]-16,u=ubuf[(sy/2)*s->chroma_width+sx/2]-128,v=vbuf[(sy/2)*s->chroma_width+sx/2]-128;int r=(298*yy+409*v+128)>>8,g=(298*yy-100*u-208*v+128)>>8,b=(298*yy+516*u+128)>>8;r=r<0?0:r>255?255:r;g=g<0?0:g>255?255:g;b=b<0?0:b>255?255:b;pixels[y*400+x]=r<<16|g<<8|b;}}
            frames++;if(frames==1)bds->print("PLAYER video frame 1\n");if(frames==30)bds->print("PLAYER video frame 30\n");ui();}
    }fail("Video decoder made no bounded progress.");return 0;
}
static int audio_bytes(u8*p,int n,int id){
    if(id>=0xc0&&id<=0xdf){plm_buffer_write(mp2_buffer,p,n);plm_samples_t*s;while((s=plm_audio_decode(mp2))){for(int i=0;i<2304;i++){float v=s->interleaved[i]*32767;pcm[i]=v>32767?32767:v<-32768?-32768:(short)v;}if(!audio(pcm,1152,plm_audio_get_samplerate(mp2),2))return 0;}return 1;}
    if(n<4)return 1;int sub=p[0];if(sub>=0x80&&sub<=0x87){p+=4;n-=4;if((u32)n>sizeof(ac3_data)-ac3_size){ac3_size=0;fail("Damaged AC-3 packet.");return 0;}memcpy(ac3_data+ac3_size,p,n);ac3_size+=n;u32 at=0;
        while(ac3_size-at>=7){int flags,rate,bitrate;int bytes=a52_syncinfo(ac3_data+at,&flags,&rate,&bitrate);if(!bytes||bytes>3840){at++;continue;}if(ac3_size-at<(u32)bytes)break;sample_t level=1;flags=A52_STEREO|A52_ADJUST_LEVEL;if(!a52_frame(ac3,ac3_data+at,&flags,&level,0)){for(int block=0;block<6;block++){if(a52_block(ac3))break;sample_t*s=a52_samples(ac3);for(int j=0;j<256;j++)for(int c=0;c<2;c++){float v=s[j+c*256]*32767;pcm[j*2+c]=v>32767?32767:v<-32768?-32768:v;}if(!audio(pcm,256,rate,2))return 0;}}at+=bytes;}
        memmove(ac3_data,ac3_data+at,ac3_size-at);ac3_size-=at;return 1;
    }
    if(sub>=0xa0&&sub<=0xa7&&n>=7){int fmt=p[5],channels=(fmt&7)+1,rate=(fmt&16)?96000:48000;if((fmt&0xe0)||channels>2){status("DVD PCM requires 16-bit mono/stereo at 48/96 kHz.");return 1;}p+=7;n-=7;int samples=n/2;for(int at=0;at<samples;){int take=samples-at;if(take>4096)take=4096;take-=take%channels;for(int i=0;i<take;i++)pcm[i]=(short)be16(p+(at+i)*2);if(!take)break;if(!audio(pcm,take/channels,rate,channels))return 0;at+=take;}return 1;}
    status("This DVD audio stream is unsupported (for example DTS).");return 1;
}
static u32 pts90(const u8*p){return ((u32)(p[0]&6)<<29)|((u32)p[1]<<22)|((u32)(p[2]&254)<<14)|((u32)p[3]<<7)|(p[4]>>1);}
static int video_step(void){
    if(suffix(path,".M2V")){int n=read_file(packet_data,32768);return n>0&&video_bytes(packet_data,n,0,0);}
    u32 code=0xffffffff;int b;for(int i=0;i<1048576;i++){b=byte();if(b<0)return 0;code=(code<<8)|b;if((code&0xffffff00)==0x100)break;if((i&8191)==0&&!ready())return 0;}
    int id=code&255;if(id==0xb9)return 0;if(id==0xba){int first=byte();if(first<0)return 0;u8 h[9];if((first&0xc0)==0x40){if(read_file(h,9)!=9)return 0;return read_file(0,h[8]&7)==(h[8]&7);}if((first&0xf0)==0x20)return read_file(0,7)==7;return 1;}
    u8 len[2];if(read_file(len,2)!=2)return 0;int n=be16(len);if(!n)return 1;if(read_file(packet_data,n)!=n){if(!request&&!quit&&!dvd)fail("Truncated MPEG packet.");return 0;}if(id!=0xbd&&(id<0xc0||id>0xef))return 1;
    u8*p=packet_data;int off=0,valid=0;u32 pts=0;if(n>=3&&(p[0]&0xc0)==0x80){if(p[0]&0x30){fail("Encrypted DVD stream: CSS decryption is not supported.");return 0;}off=3+p[2];if((p[1]&0x80)&&p[2]>=5&&n>=8){valid=1;pts=pts90(p+3);}}
    else{while(off<n&&p[off]==255)off++;if(off<n&&(p[off]&0xc0)==0x40)off+=2;if(off<n&&(p[off]&0xe0)==0x20){if(off+5>n)return 1;valid=1;pts=pts90(p+off);off+=(p[off]&16)?10:5;}else if(off<n&&p[off]==15)off++;else return 1;}
    if(off>n)return 1;p+=off;n-=off;if(id>=0xe0){if(video_id<0)video_id=id;if(video_id==id)return video_bytes(p,n,pts,valid);}
    else{if(id==0xbd&&(!n||!((p[0]>=0x80&&p[0]<=0x87)||(p[0]>=0xa0&&p[0]<=0xa7))))return 1;int key=id==0xbd&&n?0x100+p[0]:id;if(audio_id<0)audio_id=key;if(audio_id==key)return audio_bytes(p,n,id);}return 1;
}
static void skip_time(int direction){
    if(dvd){u32 pos,len;if(dvdnav_get_position(dvd,&pos,&len)==DVDNAV_STATUS_OK){u32 step=len/20;u32 dest=direction<0?(pos>step?pos-step:0):(len-pos>step?pos+step:len-1);if(dvdnav_sector_search(dvd,dest,SEEK_SET)==DVDNAV_STATUS_OK)dvd_reset();}return;}
    if(mode==MUSIC){status("MP3/MIDI seek is unavailable; Stop/Open restarts playback.");return;}
    bds->audio_stop();paused=0;
    if(mode==WAVE){u32 at=file_pos-wav_start,step=wav_rate*wav_align*10;if(direction<0)at=at>step?at-step:0;else at=wav_bytes-at>step?at+step:wav_bytes;seek_file(wav_start+at/wav_align*wav_align);}
    else if(mode==CD){u32 start=toc.tracks[track].lba;cd_lba=direction<0?(cd_lba-start>750?cd_lba-750:start):(cd_end-cd_lba>750?cd_lba+750:cd_end);}
    else if(mode==VIDEO){u32 step=file_size/20;seek_file(direction<0?(file_pos>step?file_pos-step:0):(file_size-file_pos>step?file_pos+step:file_size));mpeg2_reset(video,1);ac3_size=0;plm_audio_rewind(mp2);frames=have_pts=0;anchor=bds->ticks();}status("Seek complete.");
}
static int next_file(int direction){
    if(dvd){int r=direction>0?dvdnav_next_pg_search(dvd):dvdnav_prev_pg_search(dvd);if(r==DVDNAV_STATUS_OK)dvd_reset();return r==DVDNAV_STATUS_OK;}if(mode==CD)return cd_track(track+direction);char next[256];strcpy(next,path);int n=strlen(next);
    if(suffix(next,".VOB")&&n>5&&next[n-5]>='1'&&next[n-5]<='9'){next[n-5]+=direction;if(next[n-5]>='1'&&next[n-5]<='9'&&bds->find(next)>=0){strcpy(path,next);return start();}}
    for(int i=0;i<list_count;i++)if(!strcasecmp(list[i],path)){int j=i+direction;if(j>=0&&j<list_count){strcpy(path,list[j]);return start();}}return 0;
}
#include "dvd_play.h"
int player_main(BdsApi*a,int argc,char**argv){
    bds=a;mpeg2_accel(0);if(argc>1){strncpy(path,argv[1],255);start();}else if(!dvd_detect()&&!open_cd())request=2;
    while(!quit){poll();if(request){int action=request;request=0;if(action==1)start();else if(action==2){char chosen[256];if(!bds->pick_file(".MP3;.MID;.MIDI;.WAV;.MPG;.MPEG;.M2V;.VOB;.CUE;.IFO",chosen,sizeof(chosen))){strcpy(path,chosen);start();}}else if(action==3){if(!dvd_detect())playlist(1);}else if(action==4)open_cd();else if(action==5){if(mode==CD)cd_track(track);else if(path[0])start();if(mode&&!paused)pause_toggle();status("Stopped. Play restarts from the beginning.");}else if(action==6||action==7){if(!next_file(action==6?1:-1))status("No more tracks/files.");}else if(action==8||action==9)skip_time(action==8?1:-1);else if(action==10)playlist(0);else if(action==12)cd_track(0);else if(action==13){if(dvd){if(dvd_jump==1)dvdnav_menu_call(dvd,DVD_MENU_Root);else if(dvd_jump==2)dvdnav_button_activate(dvd,dvdnav_get_current_nav_pci(dvd));dvd_jump=0;dvd_reset();}}ui();}
        if(quit)break;if(paused||list_open||!mode){bds->sleep(10);continue;}int ok=1;
        if(mode==MUSIC){position=bds->music_control(2,0);if(!bds->music_control(0,0))ok=0;bds->sleep(10);}
        else if(mode==WAVE){u32 n=wav_start+wav_bytes-file_pos;if(n>4096)n=4096;n-=n%wav_align;u8 data[4096];if(!n||read_file(data,n)!=(int)n)ok=0;else{int samples=n/(wav_bits/8);for(int i=0;i<samples;i++)pcm[i]=wav_bits==8?((int)data[i]-128)*256:(short)(data[i*2]|data[i*2+1]<<8);position=(uint64_t)(file_pos-wav_start)*1000/(wav_rate*wav_align);ok=audio(pcm,n/wav_align,wav_rate,wav_channels);}}
        else if(mode==CD){u32 n=cd_end-cd_lba;if(n>4)n=4;if(!n)ok=0;else{u8 raw[4*2352];int r=cd_image>=0?bds->read(cd_image,cd_lba*2352,raw,n*2352):bds->cd_read(cd_device,cd_lba,n,raw)*2352;if(r!=(int)n*2352){fail("Audio CD read failed or the disc was removed.");ok=0;}else{cd_lba+=n;position=(cd_lba-toc.tracks[track].lba)*1000/75;ok=audio((short*)raw,n*588,44100,2);}}}
        else if(mode==VIDEO)ok=video_step();
        if(!ok&&!request&&!quit){int old=mode;if(old==VIDEO&&!frames&&!failure)fail("No decodable video found.");if(!failure&&old!=MUSIC){u32 end=bds->ticks()+1200;while(bds->audio_pending()&&(int)(end-bds->ticks())>0){if(!ready())break;bds->sleep(2);}if(request||quit)continue;bds->sleep(45);}if(!failure&&(old==CD||(old==VIDEO&&!dvd&&suffix(path,".VOB")))&&next_file(1))continue;if(!failure&&repeat){if(old==CD){cd_track(0);continue;}start();continue;}close_media();if(!failure)status("Playback ended.");ui();}
    }
    close_media();bds->print("PLAYER exit\n");return 0;
}
