/* DVD VM events feed the same bounded MPEG decoder as ordinary video files. */
static void dvd_reset(void){dvd_pos=dvd_len=0;dvd_still=dvd_pts_end=0;ac3_size=0;have_pts=0;frames=0;anchor=bds->ticks();bds->audio_stop();if(video)mpeg2_reset(video,1);if(mp2)plm_audio_rewind(mp2);}
static int dvd_open(const char*filename){
    char root[256];dirname(root,filename);int n=strlen(root);if(n&&root[n-1]=='/')root[n-1]=0;
    if(dvdnav_open(&dvd,root)!=DVDNAV_STATUS_OK){dvd=0;status("Cannot read DVD navigation information.");return 0;}
    dvdnav_set_readahead_flag(dvd,0);dvdnav_set_PGC_positioning_flag(dvd,1);
    dvdnav_menu_language_select(dvd,"en");dvdnav_audio_language_select(dvd,"en");dvdnav_spu_language_select(dvd,"en");
    video=mpeg2_init();ac3=a52_init(0);mp2_buffer=plm_buffer_create_with_capacity(32768);mp2=plm_audio_create_with_buffer(mp2_buffer,1);
    if(!video||!ac3||!mp2){close_media();status("Not enough memory for DVD playback.");return 0;}
    mode=VIDEO;file_size=0;failure=0;dvd_reset();strcpy(codec,"DVD menus | Arrows select, Enter plays, M returns to menu");status(codec);bds->print("DVD navigation started; autoplay\n");return 1;
}
static int dvd_detect(void){
    for(int v=0;v<32;v++){char p[256];snprintf(p,sizeof(p),"V%d:/VIDEO_TS/VIDEO_TS.IFO",v);if(bds->size(bds->find(p))>=2048){strcpy(path,p);return start();}}
    return 0;
}
static int dvd_key(int k){
    if(!dvd)return 0;if(k=='m'||k=='M'){dvd_jump=1;request=13;return 1;}
    if(!dvd_menu)return 0;pci_t*p=dvdnav_get_current_nav_pci(dvd);if(!p)return 0;
    if(k==13){dvd_jump=2;request=13;return 1;}
    if(k==KEY_UP)dvdnav_upper_button_select(dvd,p);else if(k==KEY_DOWN)dvdnav_lower_button_select(dvd,p);else if(k==KEY_LEFT)dvdnav_left_button_select(dvd,p);else if(k==KEY_RIGHT)dvdnav_right_button_select(dvd,p);else return 0;
    int32_t selected=0;dvdnav_get_current_highlight(dvd,&selected);char msg[64];snprintf(msg,sizeof(msg),"DVD menu button %d selected",selected);status(msg);return 1;
}
static void dvd_mouse(int x,int y,int click){
    static int old_x=-1,old_y=-1;if(x==old_x&&y==old_y&&!click)return;old_x=x;old_y=y;
    if(!dvd_menu||!movie_w||!movie_h||x<movie_x||y<movie_y||x>=movie_x+movie_w||y>=movie_y+movie_h)return;
    pci_t*p=dvdnav_get_current_nav_pci(dvd);if(!p)return;
    if(dvdnav_mouse_select(dvd,p,(x-movie_x)*dvd_w/movie_w,(y-movie_y)*dvd_h/movie_h)==DVDNAV_STATUS_OK&&click){dvd_jump=2;request=13;}
}
static void dvd_highlight(void){
    if(!dvd||!dvd_menu||!dvd_w||!dvd_h)return;pci_t*p=dvdnav_get_current_nav_pci(dvd);int32_t selected=0;dvdnav_highlight_area_t a;
    if(!p||dvdnav_get_current_highlight(dvd,&selected)!=DVDNAV_STATUS_OK||selected<1||dvdnav_get_highlight_area(p,selected,1,&a)!=DVDNAV_STATUS_OK)return;
    if(a.sx>a.ex||a.sy>a.ey||a.ex>=dvd_w||a.ey>=dvd_h)return;
    int x=movie_x+a.sx*movie_w/dvd_w,y=movie_y+a.sy*movie_h/dvd_h,w=(a.ex-a.sx+1)*movie_w/dvd_w,h=(a.ey-a.sy+1)*movie_h/dvd_h;
    bds->rect(x,y,w,2,0xffd25a);bds->rect(x,y+h-2,w,2,0xffd25a);bds->rect(x,y,2,h,0xffd25a);bds->rect(x+w-2,y,2,h,0xffd25a);
}
static int dvd_read(void*out,u32 n){
    unsigned got=0;int spins=0;
    while(got<n){
        if(dvd_pos<dvd_len){unsigned take=dvd_len-dvd_pos;if(take>n-got)take=n-got;if(out)memcpy((u8*)out+got,dvd_block+dvd_pos,take);dvd_pos+=take;got+=take;continue;}
        if(!ready())return got;int32_t event,len=0;
        if(dvdnav_get_next_block(dvd,dvd_block,&event,&len)!=DVDNAV_STATUS_OK){fail(dvdnav_err_to_string(dvd));return got;}
        if(event==DVDNAV_BLOCK_OK||event==DVDNAV_NAV_PACKET){if(len!=2048){fail("Invalid DVD sector length.");return got;}dvd_pos=0;dvd_len=len;spins=0;
            if(event==DVDNAV_NAV_PACKET){pci_t*p=dvdnav_get_current_nav_pci(dvd);int menu=!dvdnav_is_domain_vts(dvd);if(menu!=dvd_menu){dvd_menu=menu;if(menu)status("DVD menu: arrows / Enter or click; M returns to menu.");else status("DVD title playing; M returns to menu.");}
                if(p){if(dvd_pts_end&&p->pci_gi.vobu_s_ptm!=dvd_pts_end){have_pts=0;anchor=bds->ticks();}dvd_pts_end=p->pci_gi.vobu_e_ptm;}
            }continue;
        }
        if(event==DVDNAV_STOP)return got;
        if(event==DVDNAV_STILL_FRAME){int seconds=((dvdnav_still_event_t*)dvd_block)->length;
            if(!dvd_still){u8 end[]={0,0,1,0xb7};video_bytes(end,4,0,0);dvd_still=bds->ticks()+1;}
            if(seconds!=255&&bds->ticks()-dvd_still>=(u32)seconds*1000){dvdnav_still_skip(dvd);dvd_still=0;}
            bds->sleep(10);spins=0;continue;
        }
        if(event==DVDNAV_WAIT){u32 end=bds->ticks()+1200;while(bds->audio_pending()&&(int)(end-bds->ticks())>0){if(!ready())return got;bds->sleep(2);}dvdnav_wait_skip(dvd);}
        if(event==DVDNAV_VTS_CHANGE){dvdnav_get_video_resolution(dvd,&dvd_w,&dvd_h);if(!dvd_w||dvd_w>1024||!dvd_h||dvd_h>768){fail("Unsupported DVD dimensions.");return got;}dvd_reset();audio_id=video_id=-1;}
        if(event==DVDNAV_HOP_CHANNEL){dvd_reset();}
        if(event==DVDNAV_CELL_CHANGE){have_pts=0;anchor=bds->ticks();duration=((dvdnav_cell_change_event_t*)dvd_block)->pgc_length/90;int32_t title,part;if(dvdnav_current_title_info(dvd,&title,&part)==DVDNAV_STATUS_OK){char info[80];snprintf(info,sizeof(info),"DVD title %d chapter %d\n",title,part);bds->print(info);}}
        if(event==DVDNAV_AUDIO_STREAM_CHANGE){int stream=((dvdnav_audio_stream_change_event_t*)dvd_block)->physical;if(stream>=0){int fmt=dvdnav_audio_stream_format(dvd,stream);audio_id=fmt==0?0x180+stream:fmt==4?0x1a0+stream:fmt==2||fmt==3?0xc0+stream:-1;ac3_size=0;}}
        if(++spins>4096){fail("DVD navigation made no bounded progress.");return got;}
    }return got;
}
