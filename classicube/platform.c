/* Native BDS-OS platform adapter for upstream ClassiCube. */
#include "libc.h"
#include "bds.h"
#define CC_NO_UPDATER
#define CC_NO_DYNLIB
#define CC_NO_SOCKETS
#define CC_NO_THREADING
#define CC_NO_ENCRYPTION
#define CC_NO_OPEN
#define CC_NO_CRASHHANDLER
#define DEFAULT_COMMANDLINE_FUNC
#include "_PlatformBase.h"
#include "Utils.h"
#include "Stream.h"
BdsApi* bds;
const cc_result ReturnCode_FileShareViolation=1000000000;
const cc_result ReturnCode_FileNotFound=2,ReturnCode_PathNotFound=3,ReturnCode_DirectoryExists=17;
const char* Platform_AppNameSuffix=" BDS-OS";
cc_uint8 Platform_Flags=PLAT_FLAG_SINGLE_PROCESS|PLAT_FLAG_APP_EXIT;
cc_bool Platform_ReadonlyFilesystem;
void Platform_Log(const char* msg,int len){char line[256];while(len>0){int n=len>255?255:len;memcpy(line,msg,n);line[n]=0;bds->print(line);msg+=n;len-=n;}bds->print("\n");}
void Platform_Init(void){}
void Platform_Free(void){}
cc_result Platform_SetDefaultCurrentDirectory(void){if(bds->find("/CLASSIC")<0)return bds->mkdir("/CLASSIC")?5:0;return 0;}
cc_bool Platform_DescribeError(cc_result res,cc_string* dst){return false;}
void Platform_LoadSysFonts(void){}
void Thread_Sleep(cc_uint32 ms){bds->sleep(ms);}
cc_uint64 Stopwatch_Measure(void){return (cc_uint64)bds->ticks()*1000;}
cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg,cc_uint64 end){return end>=beg?end-beg:0;}
void DateTime_CurrentLocal(struct cc_datetime* t){int d[6];bds->date_time(d);t->year=d[0];t->month=d[1];t->day=d[2];t->hour=d[3];t->minute=d[4];t->second=d[5];t->__milli=0;}
TimeMS DateTime_CurrentUTC(void){struct cc_datetime t;DateTime_CurrentLocal(&t);static const int md[]={31,28,31,30,31,30,31,31,30,31,30,31};cc_uint64 days=0;for(int y=1;y<t.year;y++)days+=365+(y%4==0&&(y%100!=0||y%400==0));for(int m=1;m<t.month;m++)days+=md[m-1]+(m==2&&t.year%4==0&&(t.year%100!=0||t.year%400==0));return (days+t.day-1)*86400+t.hour*3600+t.minute*60+t.second;}
void Platform_EncodePath(cc_filepath* dst,const cc_string* path){char raw[600];if(path->length>190){dst->buffer[0]=0;return;}String_EncodeUtf8(raw,path);if(raw[0]=='/'||((raw[0]=='V'||raw[0]=='v')&&raw[1]>='0'&&raw[1]<='9'))strcpy(dst->buffer,raw);else{strcpy(dst->buffer,"/CLASSIC/");strcat(dst->buffer,raw);}}
void Platform_DecodePath(cc_string* dst,const cc_filepath* path){String_AppendUtf8(dst,path->buffer,strlen(path->buffer));}
void Directory_GetCachePath(cc_string* path){}
cc_result Directory_Create2(const cc_filepath* path){if(!path->buffer[0])return 22;if(bds->find(path->buffer)>=0)return ReturnCode_DirectoryExists;return bds->mkdir(path->buffer)?5:0;}
int File_Exists(const cc_filepath* path){return path->buffer[0]&&bds->size(bds->find(path->buffer))>=0;}
static cc_result enumerate(const cc_string* dir,void* obj,Directory_EnumCallback cb,int depth){if(depth>16)return 22;cc_filepath native;Platform_EncodePath(&native,dir);FsEntry e;for(int i=0;;i++){int r=bds->read_dir(native.buffer,i,&e);if(r<0)return 3;if(!r)return 0;char buf[600];cc_string p;String_InitArray(p,buf);String_AppendString(&p,dir);if(p.length)String_Append(&p,'/');String_AppendConst(&p,e.name);cb(&p,obj,e.type==2);if(e.type==2){cc_result r=enumerate(&p,obj,cb,depth+1);if(r)return r;}}}
cc_result Directory_Enum(const cc_string* dir,void* obj,Directory_EnumCallback cb){return enumerate(dir,obj,cb,0);}
static cc_result file_open(cc_file* out,const cc_filepath* path,const char* mode){if(!path->buffer[0])return 22;FILE* f=fopen(path->buffer,mode);*out=(int)f;if(!f)return errno==ENOENT?2:5;if(*mode=='a')fseek(f,0,SEEK_SET);return 0;}
cc_result File_Open(cc_file* f,const cc_filepath* path){return file_open(f,path,"r");}
cc_result File_Create(cc_file* f,const cc_filepath* path){return file_open(f,path,"w");}
cc_result File_OpenOrCreate(cc_file* f,const cc_filepath* path){return file_open(f,path,"a");}
cc_result File_Read(cc_file f,void* data,cc_uint32 count,cc_uint32* n){*n=fread(data,1,count,(FILE*)f);return ferror((FILE*)f)?5:0;}
cc_result File_Write(cc_file f,const void* data,cc_uint32 count,cc_uint32* n){*n=fwrite(data,1,count,(FILE*)f);return *n==count?0:5;}
cc_result File_Close(cc_file f){FILE* file=(FILE*)f;int writing=file->mode;char path[256];strcpy(path,file->path);cc_result res=fclose(file)?5:0;if(writing&&!res){bds->print("CLASSICUBE saved: ");bds->print(path);bds->print("\n");}return res;}
cc_result File_Seek(cc_file f,int offset,int kind){return fseek((FILE*)f,offset,kind)?22:0;}
cc_result File_Position(cc_file f,cc_uint32* pos){*pos=ftell((FILE*)f);return 0;}
cc_result File_Length(cc_file f,cc_uint32* len){*len=((FILE*)f)->size;return 0;}
cc_result Process_StartGame2(const cc_string* args,int count){return SetGameArgs(args,count);}
void Process_Exit(cc_result code){bds->mouse_capture(0);bds->print("CLASSICUBE exit\n");bds->exit(code);for(;;);}
#include "main_impl.h"
int cube_main(BdsApi* api,int argc,char** argv){bds=api;if(api->version<5){api->print("ClassiCube requires BDS API 5.\n");return -1;}api->rect(0,0,800,600,0x172c24);api->text(180,260,"ClassiCube - loading textures and world...",0xffffff);api->present();char* args[]={"CUBE.RUN","Player"};bds->print("CLASSICUBE native software renderer\n");SetupProgram(argc,argv);int r=RunProgram(argc>1?argc:2,argc>1?argv:args);Window_Free();Platform_Free();bds->print("CLASSICUBE exit\n");return r;}
