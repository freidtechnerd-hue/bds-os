/* Deliberately small CSS text layout: simple selectors, cascade and inheritance. */
typedef struct{u32 fg,bg;int bold,align,hidden,pre,margin;}Style;
typedef struct{char selector[80],declarations[512];int specificity;}Rule;
static Rule rules[96];static int rule_count;static Style style;static u32 page_background=0xe8e9db;
static u32 ink[MAX_LINES][95],paper[MAX_LINES];static u8 weight[MAX_LINES][95],alignment[MAX_LINES],indent[MAX_LINES];
static char*trim(char*s){while(isspace(*s))s++;int n=strlen(s);while(n&&isspace(s[n-1]))s[--n]=0;return s;}
static int css_color(const char*s,u32*out){
    if(*s=='#'){int n=strlen(++s);if(n!=3&&n!=6)return 0;u32 c=0;for(int i=0;i<n;i++){int d=isdigit(s[i])?s[i]-'0':tolower(s[i])-'a'+10;if(d<0||d>15)return 0;c=(c<<(n==3?8:4))|(n==3?d*17:d);}*out=c;return 1;}
    const char*names[]={"black","white","red","green","blue","navy","yellow","gray","silver","purple","teal","orange","maroon","lime","aqua","fuchsia"};u32 values[]={0,0xffffff,0xff0000,0x008000,0x0000ff,0x000080,0xffff00,0x808080,0xc0c0c0,0x800080,0x008080,0xffa500,0x800000,0x00ff00,0x00ffff,0xff00ff};for(int i=0;i<16;i++)if(!strcasecmp(s,names[i])){*out=values[i];return 1;}return 0;
}
static void declarations(Style*t,const char*css){
    int total=0;while(*css&&total++<64){char key[40],val[96];int n=0;while(*css&&*css!=':'&&*css!=';'){if(n<39)key[n++]=*css;css++;}key[n]=0;if(*css!=':'){if(*css)css++;continue;}css++;n=0;while(*css&&*css!=';'){if(n<95)val[n++]=*css;css++;}val[n]=0;if(*css)css++;char*k=trim(key),*v=trim(val);
        if(!strcasecmp(k,"color"))css_color(v,&t->fg);else if(!strcasecmp(k,"background-color")||!strcasecmp(k,"background"))css_color(v,&t->bg);
        else if(!strcasecmp(k,"display"))t->hidden=!strcasecmp(v,"none");else if(!strcasecmp(k,"font-weight"))t->bold=!strcasecmp(v,"bold")||atoi(v)>=600;
        else if(!strcasecmp(k,"text-align"))t->align=!strcasecmp(v,"center")?1:!strcasecmp(v,"right")?2:0;
        else if(!strcasecmp(k,"white-space"))t->pre=!strcasecmp(v,"pre")||!strcasecmp(v,"pre-wrap");
        else if(!strcasecmp(k,"margin-left")){int x=atoi(v);t->margin=x<0?0:x>200?200:x;}
    }
}
static void stylesheet(const char*css){
    while(*css&&rule_count<96){if(css[0]=='/'&&css[1]=='*'){const char*q=strstr(css+2,"*/");if(!q)return;css=q+2;continue;}const char*start=css;while(*css&&*css!='{')css++;if(!*css)return;int n=css-start;css++;const char*d=css;while(*css&&*css!='}')css++;int dn=css-d;if(*css)css++;if(dn>511)continue;
        const char*p=start;while(p<start+n&&rule_count<96){const char*q=p;while(q<start+n&&*q!=',')q++;int sn=q-p;if(sn<80){Rule*r=&rules[rule_count];memcpy(r->selector,p,sn);r->selector[sn]=0;char*t=trim(r->selector);memmove(r->selector,t,strlen(t)+1);memcpy(r->declarations,d,dn);r->declarations[dn]=0;r->specificity=1;for(t=r->selector;*t;t++)if(*t=='#')r->specificity+=100;else if(*t=='.')r->specificity+=10;rule_count++;}p=q+1;}
    }
}
static int css_matches(GumboElement*e,const char*s){
    const char*p=s;while(isalnum(*p)||*p=='-'||*p=='*')p++;int n=p-s;if(n&&!(n==1&&s[0]=='*')){const char*tag=gumbo_normalized_tagname(e->tag);if((int)strlen(tag)!=n||strncasecmp(tag,s,n))return 0;}
    while(*p){char kind=*p++;if(kind!='.'&&kind!='#')return 0;const char*b=p;while(isalnum(*p)||*p=='_'||*p=='-')p++;n=p-b;if(!n)return 0;GumboAttribute*a=gumbo_get_attribute(&e->attributes,kind=='#'?"id":"class");if(!a)return 0;
        if(kind=='#'){if((int)strlen(a->value)!=n||strncmp(a->value,b,n))return 0;}else{const char*c=a->value;int found=0;while(*c){while(isspace(*c))c++;const char*q=c;while(*q&&!isspace(*q))q++;if(q-c==n&&!strncmp(c,b,n))found=1;c=q;}if(!found)return 0;}
    }return 1;
}
static Style element_style(GumboElement*e,Style parent){
    Style t=parent;GumboTag tag=e->tag;if(tag==GUMBO_TAG_B||tag==GUMBO_TAG_STRONG||(tag>=GUMBO_TAG_H1&&tag<=GUMBO_TAG_H6))t.bold=1;if(tag==GUMBO_TAG_PRE)t.pre=1;if(tag==GUMBO_TAG_A)t.fg=0x245da0;
    const char*attrs[]={"color","bgcolor","align"};for(int i=0;i<3;i++){GumboAttribute*a=gumbo_get_attribute(&e->attributes,attrs[i]);if(a){if(i<2)css_color(a->value,i?&t.bg:&t.fg);else t.align=!strcasecmp(a->value,"center")?1:!strcasecmp(a->value,"right")?2:0;}}
    /* Apply rules from low to high specificity, source order for ties. */
    int ids[96],n=0;for(int i=0;i<rule_count;i++)if(css_matches(e,rules[i].selector)){int j=n++;while(j&&rules[ids[j-1]].specificity>rules[i].specificity){ids[j]=ids[j-1];j--;}ids[j]=i;}for(int i=0;i<n;i++)declarations(&t,rules[ids[i]].declarations);
    GumboAttribute*a=gumbo_get_attribute(&e->attributes,"style");if(a)declarations(&t,a->value);return t;
}
