#include "libc.h"
/* Freestanding decimal conversion for the embedded interpreter. */
double strtod(const char*s,char**end){const char*start=s;while(isspace(*s))s++;int sign=1;if(*s=='-'||*s=='+'){if(*s=='-')sign=-1;s++;}double v=0,scale=1;int digits=0;while(isdigit(*s)){v=v*10+*s++-'0';digits++;}if(*s=='.'){s++;while(isdigit(*s)){scale*=.1;v+=(*s++-'0')*scale;digits++;}}if(!digits){if(end)*end=(char*)start;return 0;}if(*s=='e'||*s=='E'){const char*p=s++;int es=1,e=0;if(*s=='-'||*s=='+'){if(*s=='-')es=-1;s++;}if(!isdigit(*s))s=p;else{while(isdigit(*s)){if(e<400)e=e*10+*s-'0';s++;}if(e>400)e=400;while(e--)v=es<0?v*.1:v*10;}}if(end)*end=(char*)s;return sign*v;}
size_t bds_js_number(double v,char*out,size_t cap){
    union{double d;unsigned long long bits;}u={v};if((u.bits&0x7ff0000000000000ULL)==0x7ff0000000000000ULL){const char*s=(u.bits&0xfffffffffffffULL)?"NaN":v<0?"-Infinity":"Infinity";return snprintf(out,cap,"%s",s);}
    int exponent=0;double a=v<0?-v:v;if(a>=1000000000.0||(a>0&&a<.000001)){while(a>=10){v*=.1;a*=.1;exponent++;}while(a<1&&a>0){v*=10;a*=10;exponent--;}}
    char b[80];snprintf(b,sizeof(b),"%.6f",v);int n=strlen(b);while(n&&b[n-1]=='0')b[--n]=0;if(n&&b[n-1]=='.')b[--n]=0;if(exponent)snprintf(b+n,sizeof(b)-n,"e%d",exponent);return snprintf(out,cap,"%s",b);
}
