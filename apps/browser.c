#include "libc.h"
#include "bds.h"
#include "gumbo.h"
#include "lodepng.h"
BdsApi*bds;
#define MAX_LINES 4096
#define MAX_LINKS 512
#define PAGE_CAP (2*1024*1024)
static int web_scheme(const char*s){return !strncmp(s,"https://",8)?8:!strncmp(s,"http://",7)?7:0;}
static char lines[MAX_LINES][95],links[MAX_LINKS][256],url[256]="/HOME.HTM",edit[256],status[160];
static int line_count,col,top,link_count,selected=-1,editing,edit_len;static u32 colors[MAX_LINES];static char*response;
static int link_line[MAX_LINKS],resources;static char page_title[80];
static char history[32][256];static int history_count,history_at=-1;
typedef struct {u32*pixels;int line,w,h;} PageImage;
static PageImage images[4];static int image_count;
static int resource(const char*,char*,int);
#include "browser_style.h"
#include "browser_script.h"

static void draw(void){
    char info[180];bds->net_info(info,sizeof(info));bds->rect(0,0,800,600,page_background);bds->rect(0,0,800,68,0x243e36);bds->text(12,10,page_title[0]?page_title:"BDS Browser | HTML, CSS + JavaScript",0xffffff);
    bds->rect(10,26,700,24,editing?0xfff4d6:0xffffff);bds->rect(718,26,70,24,0x416b58);bds->text(740,34,"Go",0xffffff);const char*address=editing?edit:url;int length=strlen(address);if(length>84)address+=length-84;bds->text(16,34,address,0x172a39);if(editing)bds->rect(16+strlen(address)*8,32,1,12,0x172a39);bds->text(12,55,info,0x9bc8a8);
    for(int i=0;i<32&&top+i<line_count;i++){int row=top+i,y=78+i*14,n=strlen(lines[row]),x=16+indent[row];if(alignment[row]==1)x=(800-n*8)/2;else if(alignment[row]==2)x=784-n*8;bds->rect(0,y-2,800,14,paper[row]);for(int j=0;j<n;j++){char c[2]={lines[row][j],0};bds->text(x+j*8,y,c,ink[row][j]);if(weight[row][j])bds->text(x+j*8+1,y,c,ink[row][j]);}}
    if(bds->version>=6)for(int i=0;i<image_count;i++){PageImage*p=images+i;int y=78+(p->line-top)*14,skip=y<78?78-y:0,h=p->h-skip;if(y<78)y=78;if(y+h>526)h=526-y;if(h>0)bds->surface_blit(p->pixels+skip*p->w,p->w,h,16,y,p->w,h);}
    bds->rect(0,538,800,62,0x243e36);bds->text(12,550,status,0xf1ead7);bds->text(12,570,"Ctrl+L address | [ back / ] next | Ctrl+C/V | Tab links | O file",0xc9e4cf);
    if(selected>=0){char t[100];snprintf(t,sizeof(t),"Link %d: %.82s",selected+1,links[selected]);bds->text(12,586,t,0xffd080);}bds->present();
}
static void newline(void){if(line_count>=MAX_LINES)return;if(col){lines[line_count][col]=0;line_count++;col=0;}}
static void append(const char*s,u32 color){while(*s&&line_count<MAX_LINES){u8 c=*s++;if(c>=128){if((c&192)==128)continue;c='?';}if(style.pre&&c=='\n'){newline();continue;}if(isspace(c)){if(!style.pre&&(!col||lines[line_count][col-1]==' '))continue;c=' ';}if(col>=92-style.margin/8)newline();if(line_count>=MAX_LINES)break;if(!col){paper[line_count]=style.bg;alignment[line_count]=style.align;indent[line_count]=style.margin;}ink[line_count][col]=color?color:style.fg;weight[line_count][col]=style.bold;lines[line_count][col++]=c;lines[line_count][col]=0;}}
static int absolute(const char*base,const char*ref,char*out){
    if(!ref||strlen(ref)>240)return -1;if(strstr(ref,"://")){if(!web_scheme(ref))return -1;strcpy(out,ref);return 0;}if(!strncmp(ref,"//",2)){if(strlen(ref)>248)return -1;strcpy(out,web_scheme(base)==8?"https:":"http:");strcat(out,ref);return 0;}
    if(!web_scheme(base)){if(ref[0]=='/'){strcpy(out,ref);return 0;}const char*p=strrchr(base,'/');int n=p?p-base+1:0;if(n+strlen(ref)>255)return -1;memcpy(out,base,n);strcpy(out+n,ref);return 0;}
    const char*authority=base+web_scheme(base);const char*slash=strchr(authority,'/');int root=slash?slash-base:strlen(base),prefix=root;
    if(ref[0]!='/'&&slash){const char*last=strrchr(slash,'/');prefix=last-base+1;}
    if(prefix+(ref[0]!='/'&&!slash)+strlen(ref)>255)return -1;memcpy(out,base,prefix);out[prefix]=0;if(ref[0]!='/'&&!slash)strcat(out,"/");strcat(out,ref);return 0;
}
static void images_free(void){for(int i=0;i<image_count;i++)free(images[i].pixels);image_count=0;}
static int inline_image(const char*ref){
    if(bds->version<6||image_count>=4||!ref)return -1;
    char*encoded=malloc(131072);if(!encoded)return -1;int n=resource(ref,encoded,131072);unsigned w=0,h=0;LodePNGState state;lodepng_state_init(&state);
    unsigned err=n>0?lodepng_inspect(&w,&h,&state,(const unsigned char*)encoded,n):1;lodepng_state_cleanup(&state);
    if(err||!w||!h||w>1024||h>1024){free(encoded);return -1;}
    unsigned char*rgba=0;err=lodepng_decode32(&rgba,&w,&h,(const unsigned char*)encoded,n);free(encoded);if(err){free(rgba);return -1;}
    int dw=w,dh=h;if(dw>480){dh=dh*480/dw;dw=480;}if(dh>224){dw=dw*224/dh;dh=224;}if(dw<1)dw=1;if(dh<1)dh=1;
    u32*scaled=malloc(dw*dh*4);if(!scaled){free(rgba);return -1;}
    for(int y=0;y<dh;y++)for(int x=0;x<dw;x++){unsigned char*p=rgba+((y*h/dh)*w+x*w/dw)*4;unsigned a=p[3],bg=style.bg;scaled[y*dw+x]=((p[0]*a+((bg>>16)&255)*(255-a))/255<<16)|((p[1]*a+((bg>>8)&255)*(255-a))/255<<8)|((p[2]*a+(bg&255)*(255-a))/255);}
    free(rgba);newline();int rows=(dh+13)/14;if(line_count+rows>=MAX_LINES){free(scaled);return -1;}
    images[image_count++]=(PageImage){scaled,line_count,dw,dh};for(int i=0;i<rows;i++){paper[line_count]=style.bg;line_count++;}
    bds->print("BROWSER inline PNG decoded\n");return 0;
}
static void render(GumboNode*n,int depth){
    if(depth>128||line_count>=MAX_LINES)return;
    if(n->type==GUMBO_NODE_TEXT||n->type==GUMBO_NODE_WHITESPACE){append(n->v.text.text,0);return;}if(n->type!=GUMBO_NODE_ELEMENT&&n->type!=GUMBO_NODE_DOCUMENT)return;
    if(n->type==GUMBO_NODE_DOCUMENT){GumboVector*v=&n->v.document.children;for(unsigned i=0;i<v->length;i++)render(v->data[i],depth+1);return;}
    GumboElement*e=&n->v.element;GumboTag tag=e->tag;if(tag==GUMBO_TAG_HEAD||tag==GUMBO_TAG_SCRIPT||tag==GUMBO_TAG_STYLE||tag==GUMBO_TAG_NOSCRIPT)return;
    Style parent=style;style=element_style(e,parent);script_style(n,&style);if(style.hidden){style=parent;return;}if(tag==GUMBO_TAG_BODY&&n->parent==document_root)page_background=style.bg;
    int block=tag==GUMBO_TAG_P||tag==GUMBO_TAG_DIV||tag==GUMBO_TAG_LI||tag==GUMBO_TAG_PRE||tag==GUMBO_TAG_TR||tag==GUMBO_TAG_H1||tag==GUMBO_TAG_H2||tag==GUMBO_TAG_H3||tag==GUMBO_TAG_H4||tag==GUMBO_TAG_BR||tag==GUMBO_TAG_HR;
    if(block)newline();if(line_count>=MAX_LINES){style=parent;return;}if(tag==GUMBO_TAG_LI)append("* ",0);if(tag==GUMBO_TAG_HR)append("------------------------------------------------------------",0);
    if(tag==GUMBO_TAG_A&&link_count<MAX_LINKS){GumboAttribute*a=gumbo_get_attribute(&e->attributes,"href");if(a&&!absolute(url,a->value,links[link_count])){char number[12];link_line[link_count]=line_count;snprintf(number,sizeof(number),"[%d] ",++link_count);append(number,0x245da0);}}
    if(tag==GUMBO_TAG_IMG){GumboAttribute*src=gumbo_get_attribute(&e->attributes,"src");if(!src||inline_image(src->value)){GumboAttribute*a=gumbo_get_attribute(&e->attributes,"alt");append("[Image: ",0);append(a?a->value:"picture",0);append("] ",0);}}
    if(tag==GUMBO_TAG_H1||tag==GUMBO_TAG_H2||tag==GUMBO_TAG_H3)colors[line_count]=0x206749;
    int html=0;const char*replacement=script_text(n,&html);
    if(replacement){if(html){GumboOutput*g=gumbo_parse(replacement);render(g->root,depth+1);gumbo_destroy_output(&kGumboDefaultOptions,g);}else append(replacement,0);}else for(unsigned i=0;i<e->children.length;i++)render(e->children.data[i],depth+1);
    if(block)newline();style=parent;
}
static void*parser_alloc(void*arg,size_t n){void*p=malloc(n);if(!p){bds->print("BROWSER: page exceeded available memory\n");exit(1);}return p;}
static void parser_free(void*arg,void*p){free(p);}
static void css_walk(GumboNode*n,int depth){if(depth>64||n->type!=GUMBO_NODE_ELEMENT)return;GumboElement*e=&n->v.element;
    if(e->tag==GUMBO_TAG_STYLE){char*css=malloc(16384);if(css){css[0]=0;node_text(n,css,16384,0);stylesheet(css);free(css);}}
    if(e->tag==GUMBO_TAG_LINK){GumboAttribute*rel=gumbo_get_attribute(&e->attributes,"rel"),*href=gumbo_get_attribute(&e->attributes,"href");if(rel&&href&&!strcasecmp(rel->value,"stylesheet")){char*css=malloc(16384);if(css){if(resource(href->value,css,16384)>=0)stylesheet(css);free(css);}}}
    if(e->tag==GUMBO_TAG_TITLE){page_title[0]=0;node_text(n,page_title,sizeof(page_title),0);}
    for(unsigned i=0;i<e->children.length;i++)css_walk(e->children.data[i],depth+1);
}
static void page(const char*html,int size,int plain){
    images_free();
    memset(lines,0,sizeof(lines));memset(colors,0,sizeof(colors));line_count=col=top=link_count=0;selected=-1;
    page_background=0xe8e9db;style=(Style){0x1b2926,page_background,0,0,0,0,0};rule_count=resources=dom_count=script_count=script_errors=0;page_title[0]=0;
    if(plain){for(int i=0;i<size&&line_count<MAX_LINES;i++){if(html[i]=='\n')newline();else{char s[2]={html[i],0};append(s,0);}}}
    else{GumboOptions options=kGumboDefaultOptions;options.allocator=parser_alloc;options.deallocator=parser_free;options.max_errors=20;GumboOutput*g=gumbo_parse_with_options(&options,html,size);script_init(g->root);css_walk(g->root,0);js_set(js,document,"title",js_mkstr(js,page_title,strlen(page_title)));script_walk(g->root,0);const char*title=js_string(js_get(js,document,"title"));snprintf(page_title,sizeof(page_title),"%.79s",title);render(g->root,0);if(written_length){GumboOutput*w=gumbo_parse(written);render(w->root,0);gumbo_destroy_output(&kGumboDefaultOptions,w);}gumbo_destroy_output(&options,g);}
    newline();if(!line_count){append("This page has no readable text.",0);newline();}
    snprintf(status,sizeof(status),"Loaded %d bytes, %d links | JS %d scripts / %d errors | R reload",size,link_count,script_count,script_errors);bds->print("BROWSER page rendered: ");bds->print(url);bds->print("\n");draw();
}
static int header(const char*data,int length,const char*key,char*out,int cap){
    const char*p=data;int keylen=strlen(key);while(p<data+length){const char*end=p;while(end<data+length&&*end!='\n')end++;if(end-p>keylen&&!strncasecmp(p,key,keylen)&&p[keylen]==':'){p+=keylen+1;while(p<end&&(*p==' '||*p=='\t'))p++;int n=end-p;while(n&&isspace(p[n-1]))n--;if(n>=cap)return -1;memcpy(out,p,n);out[n]=0;return 1;}p=end+1;}return 0;
}
static int chunk_decode(char*body,int n){int in=0,out=0;while(in<n){unsigned size=0;int digits=0;while(in<n&&isprint(body[in])&&body[in]!=';'&&body[in]!='\r'){int c=tolower(body[in++]),v=isdigit(c)?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;if(v<0||digits++>7)return -1;size=size*16+v;}if(!digits)return -1;while(in<n&&body[in]!='\n')in++;if(in==n)return -1;in++;if(!size){body[out]=0;return out;}if(size>(unsigned)(n-in)||n-in-(int)size<2)return -1;memmove(body+out,body+in,size);out+=size;in+=size;if(body[in++]!='\r'||body[in++]!='\n')return -1;}return -1;}
static int resource(const char*ref,char*out,int cap){
    char target[256];if(resources++>=8||absolute(url,ref,target))return -1;
    if(web_scheme(target)){if(!web_scheme(url))return -1;const char*a=strchr(url+web_scheme(url),'/'),*b=strchr(target+web_scheme(target),'/');int an=a?a-url:strlen(url),bn=b?b-target:strlen(target);if(an!=bn||strncasecmp(url,target,an))return -1;char*raw=malloc(cap+2048);if(!raw)return -1;int n=bds->http_get(target,raw,cap+2048),result=-1;char*body=n>0?strstr(raw,"\r\n\r\n"):0;if(body&&atoi(raw+9)==200){int head=body-raw;body+=4;int size=n-(body-raw);char value[80];int encoded=header(raw,head,"Content-Encoding",value,sizeof(value));if(encoded==1&&strcasecmp(value,"identity"))goto done;if(header(raw,head,"Transfer-Encoding",value,sizeof(value))==1){if(strcasecmp(value,"chunked"))goto done;size=chunk_decode(body,size);}if(size>=0&&size<cap){memcpy(out,body,size);out[size]=0;result=size;}}done:free(raw);return result;}
    int f=bds->find(target),n=bds->size(f);if(n<0||n>=cap||bds->read(f,0,out,n)!=n)return -1;out[n]=0;return n;
}
static void clear_page(void){images_free();line_count=col=top=link_count=0;selected=-1;page_title[0]=0;page_background=0xe8e9db;memset(lines,0,sizeof(lines));}
static int local_url(const char*s){return *s=='/'||((s[0]=='V'||s[0]=='v')&&isdigit(s[1])&&strchr(s,':'));}
static int load(const char*target){
    if(strlen(target)>248)return -1;char clean[256];strcpy(clean,target);char*t=trim(clean);if(!*t)return -1;if(!local_url(t)&&!strstr(t,"://")){strcpy(url,"http://");strcat(url,t);}else strcpy(url,t);clear_page();strcpy(status,"Loading... ESC cancels the network request.");draw();
    if(local_url(url)){int f=bds->find(url),n=bds->size(f);if(n<0||n>=PAGE_CAP){strcpy(status,"Cannot open local page (limit 2 MiB).");draw();return -1;}if(bds->read(f,0,response,n)!=n)return -1;response[n]=0;int len=strlen(url);page(response,n,len>=4&&!strcasecmp(url+len-4,".TXT"));return 0;}
    for(int redirect=0;redirect<5;redirect++){
        int n=bds->http_get(url,response,PAGE_CAP);if(n<0){const char*s=n==-1?"No Ethernet link. Check the adapter and cable.":n==-2?"Use an http:// or https:// URL.":n==-3?"DNS lookup failed.":n==-5?"Page exceeds the 2 MiB limit.":n==-6?"Request cancelled.":n==-7?"HTTPS certificate, handshake or secure connection failed.":n==-8?"HTTPS needs CPU RDRAND secure randomness. Enable a supported VM CPU.":n==-9?"HTTPS needs a valid UTC hardware date and time.":"Network request timed out or connection failed.";strcpy(status,s);bds->print("BROWSER fetch error: ");bds->print(s);bds->print("\n");draw();return -1;}
        char*body=strstr(response,"\r\n\r\n");if(!body||body-response>8192||strncmp(response,"HTTP/1.",7)){strcpy(status,"Invalid HTTP response.");draw();return -1;}int head=body-response;body+=4;int size=n-(body-response),code=atoi(response+9);char value[256]={0};
        if(code==301||code==302||code==303||code==307||code==308){char next[256];if(header(response,head,"Location",value,sizeof(value))!=1||absolute(url,value,next)||!web_scheme(next)){strcpy(status,"Redirect has an unsupported URL.");draw();return -1;}strcpy(url,next);continue;}
        if(code!=200){snprintf(status,sizeof(status),"Server returned HTTP %d.",code);draw();return -1;}
        if(header(response,head,"Content-Encoding",value,sizeof(value))==1&&strcasecmp(value,"identity")){strcpy(status,"Compressed responses are not supported.");draw();return -1;}
        if(header(response,head,"Transfer-Encoding",value,sizeof(value))==1){if(strcasecmp(value,"chunked")||(size=chunk_decode(body,size))<0){strcpy(status,"Unsupported or damaged transfer encoding.");draw();return -1;}}
        if(header(response,head,"Content-Length",value,sizeof(value))==1&&atoi(value)!=size){strcpy(status,"Incomplete HTTP response.");draw();return -1;}
        value[0]=0;int plain=header(response,head,"Content-Type",value,sizeof(value))==1&&!strncasecmp(value,"text/plain",10);if(value[0]&&strncasecmp(value,"text/",5)){strcpy(status,"This browser displays HTML and plain text pages.");draw();return -1;}
        page(body,size,plain);return 0;
    }strcpy(status,"Too many redirects.");draw();return -1;
}
static void navigate(const char*target){
    if(load(target))return;
    if(history_at>=0&&!strcmp(history[history_at],url))return;
    history_count=history_at+1;if(history_count==32){memmove(history,history+1,31*256);history_count=31;}
    strcpy(history[history_count],url);history_at=history_count++;
}
int app_main(BdsApi*api,int argc,char**argv){
    bds=api;if(bds->version<4){bds->print("BROWSER requires BDS API v4\n");return 1;}response=malloc(PAGE_CAP);if(!response)return 1;bds->print("BROWSER native Gumbo HTML5 / lwIP / Elk ready\n");navigate(argc>1?argv[1]:"/HOME.HTM");int previous=0;
    for(;;){int k=bds->key(0),x,y,buttons=bds->pointer(&x,&y);if((buttons&1)&&!previous){if(y>=26&&y<50&&x>=10&&x<710){editing=1;edit_len=0;edit[0]=0;draw();}else if(y>=26&&y<50&&x>=718){if(editing)k=13;}else if(y>=78&&y<526){int row=top+(y-78)/14;for(int i=0;i<link_count;i++)if(link_line[i]==row){editing=0;selected=i;k=13;break;}}}previous=buttons&1;
        if(k==KEY_ADDRESS||k==KEY_F1+1){editing=1;edit_len=0;edit[0]=0;draw();continue;}
        if(k==KEY_COPY&&bds->version>=7){const char*s=editing?edit:url;bds->clipboard_set(s,strlen(s));strcpy(status,"Address copied to shared clipboard");draw();continue;}
        if(k==KEY_PASTE&&bds->version>=7){char clip[4096];int n=bds->clipboard_get(clip,sizeof(clip));if(n>0&&n<=248){int valid=1;for(int i=0;i<n;i++)if((u8)clip[i]<32||(u8)clip[i]>=127)valid=0;if(valid){strcpy(edit,clip);edit_len=n;editing=1;strcpy(status,"Pasted address. Enter or Go opens it.");}}else strcpy(status,"Address clipboard limit: 248 bytes");draw();continue;}
        if(editing){if(k==27)editing=0;else if(k==KEY_SELECT_ALL){edit_len=0;edit[0]=0;}else if(k==13){editing=0;navigate(edit);}else if(k==8&&edit_len)edit[--edit_len]=0;else if(k>=32&&k<127&&edit_len<248){edit[edit_len++]=k;edit[edit_len]=0;}if(k)draw();}
        else if(k==27||k==KEY_F10)break;
        else if(k=='l'||k=='L'){editing=1;edit_len=0;edit[0]=0;draw();}
        else if(k==KEY_UP){if(top)top--;draw();}else if(k==KEY_DOWN){if(top+32<line_count)top++;draw();}else if(k==' '){top+=28;if(top>line_count-1)top=line_count?line_count-1:0;draw();}
        else if(k=='['||k==']'){int next=history_at+(k=='['?-1:1);if(next>=0&&next<history_count){char target[256];strcpy(target,history[next]);if(!load(target))history_at=next;}}
        else if(k==9&&link_count){selected=(selected+1)%link_count;draw();}else if(k==13&&selected>=0){char target[256];strcpy(target,links[selected]);navigate(target);}
        else if(k=='H')navigate("/HOME.HTM");else if(k=='r'||k=='R'){char target[256];strcpy(target,url);load(target);}
        else if(k=='o'||k=='O'){char p[256];if(!bds->pick_file(".HTM;.HTML;.TXT",p,sizeof(p)))navigate(p);}
        else if(k==KEY_OPEN){char p[256];if(!bds->opened_path(p,sizeof(p)))navigate(p);}
        else if(k>=32&&k<127){editing=1;edit_len=1;edit[0]=k;edit[1]=0;draw();}
        if(!k){bds->present();bds->sleep(40);}
    }images_free();free(response);return 0;
}
