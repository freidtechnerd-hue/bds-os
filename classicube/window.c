#include "bds.h"
#include "_WindowBase.h"
#include "String_.h"
#include "Bitmap.h"
#include "Errors.h"
#include "Formats.h"
#include "Chat.h"
#include "Game.h"
#include "libc.h"
extern BdsApi* bds;
static int frame_count, raw_x, raw_y, pointer_x=-1, pointer_y=-1, button_state;
static int button_queue[64], button_count;
static int quality=1;static const int render_w[]={320,400,640},render_h[]={240,300,480};
/* Poll hardware during rasterization; dispatch game events only between frames. */
void Platform_BDS_Poll(void){
    int x,y,b;
    if(Input.RawMode){b=bds->mouse_relative(&x,&y);raw_x+=x;raw_y+=y;}
    else{b=bds->pointer(&x,&y);pointer_x=x<0?-1:x*Window_Main.Width/800;pointer_y=y<0?-1:y*Window_Main.Height/600;}
    if(b!=button_state){button_state=b;if(button_count<64)button_queue[button_count++]=b;}
}

static void Cursor_GetRawPos(int*x,int*y){bds->pointer(x,y);*x=*x*Window_Main.Width/800;*y=*y*Window_Main.Height/600;}
static void Cursor_DoSetVisible(cc_bool visible){}
void Cursor_SetPosition(int x,int y){}
void Window_PreInit(void){}
void Window_Init(void){char q=0;int f=bds->find("/CLASSIC/BDS.CFG");if(bds->read(f,0,&q,1)==1&&q>='0'&&q<='2')quality=q-'0';DisplayInfo.Width=render_w[quality];DisplayInfo.Height=render_h[quality];DisplayInfo.Depth=32;DisplayInfo.ScaleX=DisplayInfo.ScaleY=render_w[quality]/800.0f;}
void Window_Free(void){bds->mouse_capture(0);}
static void create(void){Window_Main.Width=render_w[quality];Window_Main.Height=render_h[quality];Window_Main.Focused=Window_Main.Exists=true;Window_Main.UIScaleX=DEFAULT_UI_SCALE_X;Window_Main.UIScaleY=DEFAULT_UI_SCALE_Y;}
void Window_Create2D(int w,int h){create();}
void Window_Create3D(int w,int h){create();}
void Window_Destroy(void){Window_Main.Exists=false;bds->mouse_capture(0);}
void Window_SetTitle(const cc_string* title){}
void Clipboard_GetText(cc_string* value){}
void Clipboard_SetText(const cc_string* value){}
int Window_GetWindowState(void){return WINDOW_STATE_NORMAL;}
cc_result Window_EnterFullscreen(void){bds->fullscreen(1);return 0;}
cc_result Window_ExitFullscreen(void){bds->fullscreen(0);return 0;}
int Window_IsObscured(void){return 0;}
void Window_Show(void){}
void Window_SetSize(int w,int h){Window_Main.Width=w;Window_Main.Height=h;Event_RaiseVoid(&WindowEvents.Resized);}
void Window_RequestClose(void){Event_RaiseVoid(&WindowEvents.Closing);}
static int mapped(int k){if(k>='a'&&k<='z')return k-32;if((k>='A'&&k<='Z')||(k>='0'&&k<='9'))return k;if(k>=KEY_F1&&k<=KEY_F10)return CCKEY_F1+k-KEY_F1;switch(k){case 27:return CCKEY_ESCAPE;case 13:return CCKEY_ENTER;case 8:return CCKEY_BACKSPACE;case 9:return CCKEY_TAB;case ' ':return CCKEY_SPACE;case KEY_LEFT:return CCKEY_LEFT;case KEY_RIGHT:return CCKEY_RIGHT;case KEY_UP:return CCKEY_UP;case KEY_DOWN:return CCKEY_DOWN;case '-':return CCKEY_MINUS;case '=':return CCKEY_EQUALS;case '/':return CCKEY_SLASH;case '.':return CCKEY_PERIOD;case ',':return CCKEY_COMMA;default:return 0;}}
void Window_ProcessEvents(float delta){Platform_BDS_Poll();int k;while((k=bds->keyevent())){int raw=k&~KEY_RELEASE;if(raw==KEY_OPEN){char path[256];if(!bds->opened_path(path,sizeof(path))){cc_string p=String_FromReadonly(path);Map_LoadFrom(&p);}continue;}if(raw==KEY_F1+8&&!(k&KEY_RELEASE)){quality=(quality+1)%3;Window_SetSize(render_w[quality],render_h[quality]);char q='0'+quality;bds->write("/CLASSIC/BDS.CFG",&q,1);Chat_AddRaw(quality==0?"&ePerformance: 320x240":quality==1?"&eBalanced: 400x300":"&eDetail: 640x480");bds->print("CLASSICUBE quality changed\n");continue;}if(raw==KEY_F1+7&&!(k&KEY_RELEASE)){Game_SetFpsLimit(Game_FpsLimit==FPS_LIMIT_60?FPS_LIMIT_NONE:FPS_LIMIT_60);Chat_AddRaw(Game_FpsLimit==FPS_LIMIT_60?"&eFrame limit: 60 FPS":"&eFrame limit: unlimited");continue;}if(raw==KEY_F10&&!(k&KEY_RELEASE)){Window_RequestClose();continue;}int m=mapped(raw);if(m){if(k&KEY_RELEASE)Input_SetReleased(m);else Input_SetPressed(m);}if(!(k&KEY_RELEASE)&&raw>=32&&raw<127)Event_RaiseInt(&InputEvents.Press,raw);}while(bds->key(0));int mods=bds->modifiers();Input_SetNonRepeatable(CCKEY_LSHIFT,mods&1);Input_SetNonRepeatable(CCKEY_LCTRL,mods&2);Input_SetNonRepeatable(CCKEY_LALT,mods&4);if(Input.RawMode){if(raw_x||raw_y)Event_RaiseRawMove(&PointerEvents.RawMoved,raw_x,raw_y);raw_x=raw_y=0;}else if(pointer_x>=0&&pointer_y>=0)Pointer_SetPosition(0,pointer_x,pointer_y);for(int i=0;i<button_count;i++){int b=button_queue[i];Input_SetNonRepeatable(CCMOUSE_L,b&1);Input_SetNonRepeatable(CCMOUSE_R,b&2);Input_SetNonRepeatable(CCMOUSE_M,b&4);}button_count=0;}
void Gamepads_PreInit(void){}
void Gamepads_Init(void){}
void Gamepads_Process(float delta){}
static void ShowDialogCore(const char* title,const char* msg){Platform_LogConst(title);Platform_LogConst(msg);}
cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args){char path[256];if(bds->pick_file(".CW;.DAT;.LVL;.ZIP",path,sizeof(path)))return ERR_NOT_SUPPORTED;cc_string p=String_FromReadonly(path);args->Callback(&p);return 0;}
cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args){return ERR_NOT_SUPPORTED;}
void Window_AllocFramebuffer(struct Bitmap* bmp,int w,int h){bmp->scan0=Mem_Alloc(w*h,4,"window pixels");bmp->width=w;bmp->height=h;}
void Window_DrawFramebuffer(Rect2D r,struct Bitmap* bmp){
    static u32 sample_start,previous,worst,present_ms;static int samples,last_w;
    if(last_w!=bmp->width){frame_count=samples=0;worst=present_ms=0;last_w=bmp->width;}
    u32 before=bds->ticks();
    if(bds->version>=7)bds->frame_submit(bmp->scan0,bmp->width,bmp->height);else bds->blit(bmp->scan0,bmp->width,bmp->height);frame_count++;
    if(frame_count==30)bds->print("CLASSICUBE frame 30\n");
    if(frame_count==100)bds->print("CLASSICUBE frame 100\n");
    u32 now=bds->ticks();present_ms+=now-before;
    /* Exclude loading and mesh warm-up. Report actual presented frames. */
    if(frame_count<=120){sample_start=previous=now;present_ms=0;return;}
    u32 delta=now-previous;previous=now;if(delta>worst)worst=delta;
    if(++samples==120){char line[160];u32 elapsed=now-sample_start;
        snprintf(line,sizeof(line),"CLASSICUBE PERF frames=120 ms=%u fps10=%u worst_ms=%u size=%dx%d present_ms=%u\n",elapsed,elapsed?1200000/elapsed:0,worst,bmp->width,bmp->height,present_ms);
        bds->print(line);sample_start=now;samples=0;worst=present_ms=0;
    }
}
void Window_FreeFramebuffer(struct Bitmap* bmp){Mem_Free(bmp->scan0);bmp->scan0=0;}
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args){}
void OnscreenKeyboard_SetText(const cc_string* text){}
void OnscreenKeyboard_Close(void){}
void Window_LockLandscapeOrientation(cc_bool lock){}
void Window_EnableRawMouse(void){raw_x=raw_y=0;Input.RawMode=true;bds->mouse_capture(1);}
void Window_UpdateRawMouse(void){}
void Window_DisableRawMouse(void){raw_x=raw_y=0;Input.RawMode=false;bds->mouse_capture(0);}
