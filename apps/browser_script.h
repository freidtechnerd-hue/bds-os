/* Elk runs page scripts in fixed memory, with an execution and recursion budget.
   Only this small DOM bridge is exposed; scripts have no native file/network API. */
#include "elk.h"
static struct js*js;static jsval_t document;
static u8 js_memory[256*1024];static int script_count,script_errors;
typedef struct{GumboNode*node;jsval_t object;char original[512];}DomRef;
static DomRef dom[48];static int dom_count;static GumboNode*document_root;
static char written[8192];static int written_length;
static const char*js_string(jsval_t value){size_t n;char*s=js_getstr(js,value,&n);return s?s:js_str(js,value);}
static void node_text(GumboNode*n,char*out,int cap,int depth){if(depth>64)return;if(n->type==GUMBO_NODE_TEXT){int a=strlen(out),b=strlen(n->v.text.text);if(b>=cap-a)b=cap-a-1;if(b>0){memcpy(out+a,n->v.text.text,b);out[a+b]=0;}}else if(n->type==GUMBO_NODE_ELEMENT){for(unsigned i=0;i<n->v.element.children.length;i++)node_text(n->v.element.children.data[i],out,cap,depth+1);}}
static GumboNode*find_id(GumboNode*n,const char*id,int depth){if(depth>64||n->type!=GUMBO_NODE_ELEMENT)return 0;GumboAttribute*a=gumbo_get_attribute(&n->v.element.attributes,"id");if(a&&!strcmp(a->value,id))return n;for(unsigned i=0;i<n->v.element.children.length;i++){GumboNode*r=find_id(n->v.element.children.data[i],id,depth+1);if(r)return r;}return 0;}
static jsval_t get_element(struct js*j,jsval_t*args,int n){
    if(n!=1||js_type(args[0])!=JS_STR)return js_mknull();GumboNode*node=find_id(document_root,js_string(args[0]),0);if(!node)return js_mknull();for(int i=0;i<dom_count;i++)if(dom[i].node==node)return dom[i].object;if(dom_count==48)return js_mknull();
    DomRef*d=dom+dom_count++;d->node=node;d->original[0]=0;node_text(node,d->original,sizeof(d->original),0);d->object=js_mkobj(j);js_set(j,d->object,"textContent",js_mkstr(j,d->original,strlen(d->original)));js_set(j,d->object,"innerHTML",js_mkundef());
    /* Elk assigns existing properties. Define the supported style setters. */
    jsval_t style=js_mkobj(j);js_set(j,style,"color",js_mkundef());js_set(j,style,"backgroundColor",js_mkundef());js_set(j,style,"display",js_mkundef());js_set(j,d->object,"style",style);return d->object;
}
static jsval_t document_write(struct js*j,jsval_t*args,int n){for(int i=0;i<n;i++){const char*s=js_string(args[i]);int len=strlen(s);if(len>8191-written_length)return js_mkerr(j,"document.write limit");memcpy(written+written_length,s,len);written_length+=len;written[written_length]=0;}return js_mkundef();}
static jsval_t document_writeln(struct js*j,jsval_t*args,int n){jsval_t v=document_write(j,args,n);if(js_type(v)==JS_ERR)return v;if(written_length>=8191)return js_mkerr(j,"document.write limit");written[written_length++]='\n';written[written_length]=0;return v;}
static jsval_t console_log(struct js*j,jsval_t*args,int n){bds->print("JS: ");for(int i=0;i<n;i++){bds->print(js_string(args[i]));bds->print(" ");}bds->print("\n");return js_mkundef();}
static void script_eval(const char*code){if(script_count>=16)return;script_count++;js_setbudget(js,20000);jsval_t result=js_eval(js,code,strlen(code));if(js_type(result)==JS_ERR){script_errors++;bds->print("BROWSER script error: ");bds->print(js_str(js,result));bds->print("\n");}}
static void script_init(GumboNode*root){
    document_root=root;dom_count=written_length=script_count=script_errors=0;written[0]=page_title[0]=0;js=js_create(js_memory,sizeof(js_memory));js_setmaxcss(js,8192);js_setgct(js,sizeof(js_memory));document=js_mkobj(js);js_set(js,js_glob(js),"document",document);js_set(js,document,"write",js_mkfun(document_write));js_set(js,document,"writeln",js_mkfun(document_writeln));js_set(js,document,"getElementById",js_mkfun(get_element));js_set(js,document,"title",js_mkstr(js,"BDS Browser",11));jsval_t console=js_mkobj(js);js_set(js,js_glob(js),"console",console);js_set(js,console,"log",js_mkfun(console_log));
}
static void script_walk(GumboNode*n,int depth){
    if(depth>64||n->type!=GUMBO_NODE_ELEMENT)return;GumboElement*e=&n->v.element;
    if(e->tag==GUMBO_TAG_SCRIPT&&script_count<16){GumboAttribute*t=gumbo_get_attribute(&e->attributes,"type");if(t&&strcmp(t->value,"text/javascript")&&strcmp(t->value,"application/javascript"))return;GumboAttribute*a=gumbo_get_attribute(&e->attributes,"src");char*code=malloc(16384);if(!code)return;code[0]=0;if(a){if(resource(a->value,code,16384)<0){free(code);return;}}else node_text(n,code,16384,0);script_eval(code);free(code);return;}
    for(unsigned i=0;i<e->children.length;i++)script_walk(e->children.data[i],depth+1);
    if(e->tag==GUMBO_TAG_BODY){GumboAttribute*a=gumbo_get_attribute(&e->attributes,"onload");if(a)script_eval(a->value);}
}
static const char*script_text(GumboNode*n,int*html){for(int i=0;i<dom_count;i++)if(dom[i].node==n){jsval_t v=js_get(js,dom[i].object,"innerHTML");if(js_type(v)==JS_STR){*html=1;return js_string(v);}v=js_get(js,dom[i].object,"textContent");if(js_type(v)==JS_STR&&strcmp(js_string(v),dom[i].original)){*html=0;return js_string(v);}}return 0;}
static void script_style(GumboNode*n,Style*t){for(int i=0;i<dom_count;i++)if(dom[i].node==n){jsval_t st=js_get(js,dom[i].object,"style"),v=js_get(js,st,"color");if(js_type(v)==JS_STR)css_color(js_string(v),&t->fg);v=js_get(js,st,"backgroundColor");if(js_type(v)==JS_STR)css_color(js_string(v),&t->bg);v=js_get(js,st,"display");if(js_type(v)==JS_STR)t->hidden=!strcmp(js_string(v),"none");}}
