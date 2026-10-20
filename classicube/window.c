#include "bds.h"
#include "_WindowBase.h"
#include "String_.h"
#include "Bitmap.h"
#include "Errors.h"
#include "Formats.h"
extern BdsApi* bds;
static int frame_count, raw_x, raw_y, pointer_x=-1, pointer_y=-1, button_state;
static int button_queue[64], button_count;
/* Poll hardware during rasterization; dispatch game events only between frames. */
void Platform_BDS_Poll(void){
    int x,y,b;
    if(Input.RawMode){b=bds->mouse_relative(&x,&y);raw_x+=x;raw_y+=y;}
    else{b=bds->pointer(&x,&y);pointer_x=x<0?-1:x/2;pointer_y=y<0?-1:y/2;}
    if(b!=button_state){button_state=b;if(button_count<64)button_queue[button_count++]=b;}
}

static void Cursor_GetRawPos(int*x,int*y){bds->pointer(x,y);*x/=2;*y/=2;}
static void Cursor_DoSetVisible(cc_bool visible){}
void Cursor_SetPosition(int x,int y){}
void Window_PreInit(void){}
void Window_Init(void){DisplayInfo.Width=400;DisplayInfo.Height=300;DisplayInfo.Depth=32;DisplayInfo.ScaleX=DisplayInfo.ScaleY=0.5f;}
void Window_Free(void){bds->mouse_capture(0);}
static void create(void){Window_Main.Width=400;Window_Main.Height=300;Window_Main.Focused=Window_Main.Exists=true;Window_Main.UIScaleX=DEFAULT_UI_SCALE_X;Window_Main.UIScaleY=DEFAULT_UI_SCALE_Y;}
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
void Window_SetSize(int w,int h){}
void Window_RequestClose(void){Event_RaiseVoid(&WindowEvents.Closing);}
static int mapped(int k){if(k>='a'&&k<='z')return k-32;if((k>='A'&&k<='Z')||(k>='0'&&k<='9'))return k;if(k>=KEY_F1&&k<=KEY_F10)return CCKEY_F1+k-KEY_F1;switch(k){case 27:return CCKEY_ESCAPE;case 13:return CCKEY_ENTER;case 8:return CCKEY_BACKSPACE;case 9:return CCKEY_TAB;case ' ':return CCKEY_SPACE;case KEY_LEFT:return CCKEY_LEFT;case KEY_RIGHT:return CCKEY_RIGHT;case KEY_UP:return CCKEY_UP;case KEY_DOWN:return CCKEY_DOWN;case '-':return CCKEY_MINUS;case '=':return CCKEY_EQUALS;case '/':return CCKEY_SLASH;case '.':return CCKEY_PERIOD;case ',':return CCKEY_COMMA;default:return 0;}}
void Window_ProcessEvents(float delta){Platform_BDS_Poll();int k;while((k=bds->keyevent())){int raw=k&~KEY_RELEASE;if(raw==KEY_OPEN){char path[256];if(!bds->opened_path(path,sizeof(path))){cc_string p=String_FromReadonly(path);Map_LoadFrom(&p);}continue;}if(raw==KEY_F10&&!(k&KEY_RELEASE)){Window_RequestClose();continue;}int m=mapped(raw);if(m){if(k&KEY_RELEASE)Input_SetReleased(m);else Input_SetPressed(m);}if(!(k&KEY_RELEASE)&&raw>=32&&raw<127)Event_RaiseInt(&InputEvents.Press,raw);}while(bds->key(0));int mods=bds->modifiers();Input_SetNonRepeatable(CCKEY_LSHIFT,mods&1);Input_SetNonRepeatable(CCKEY_LCTRL,mods&2);Input_SetNonRepeatable(CCKEY_LALT,mods&4);if(Input.RawMode){if(raw_x||raw_y)Event_RaiseRawMove(&PointerEvents.RawMoved,raw_x,raw_y);raw_x=raw_y=0;}else if(pointer_x>=0&&pointer_y>=0)Pointer_SetPosition(0,pointer_x,pointer_y);for(int i=0;i<button_count;i++){int b=button_queue[i];Input_SetNonRepeatable(CCMOUSE_L,b&1);Input_SetNonRepeatable(CCMOUSE_R,b&2);Input_SetNonRepeatable(CCMOUSE_M,b&4);}button_count=0;}
void Gamepads_PreInit(void){}
void Gamepads_Init(void){}
void Gamepads_Process(float delta){}
static void ShowDialogCore(const char* title,const char* msg){Platform_LogConst(title);Platform_LogConst(msg);}
cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args){char path[256];if(bds->pick_file(".CW;.DAT;.LVL;.ZIP",path,sizeof(path)))return ERR_NOT_SUPPORTED;cc_string p=String_FromReadonly(path);args->Callback(&p);return 0;}
cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args){return ERR_NOT_SUPPORTED;}
void Window_AllocFramebuffer(struct Bitmap* bmp,int w,int h){bmp->scan0=Mem_Alloc(w*h,4,"window pixels");bmp->width=w;bmp->height=h;}
void Window_DrawFramebuffer(Rect2D r,struct Bitmap* bmp){bds->blit(bmp->scan0,bmp->width,bmp->height);frame_count++;if(frame_count==30)bds->print("CLASSICUBE frame 30\n");if(frame_count==100)bds->print("CLASSICUBE frame 100\n");}
void Window_FreeFramebuffer(struct Bitmap* bmp){Mem_Free(bmp->scan0);bmp->scan0=0;}
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args){}
void OnscreenKeyboard_SetText(const cc_string* text){}
void OnscreenKeyboard_Close(void){}
void Window_LockLandscapeOrientation(cc_bool lock){}
void Window_EnableRawMouse(void){raw_x=raw_y=0;Input.RawMode=true;bds->mouse_capture(1);}
void Window_UpdateRawMouse(void){}
void Window_DisableRawMouse(void){raw_x=raw_y=0;Input.RawMode=false;bds->mouse_capture(0);}
