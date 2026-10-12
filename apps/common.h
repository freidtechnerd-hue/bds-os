#include "bds.h"
static int eq(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static int len(const char*s){int n=0;while(s[n])n++;return n;}
static int num(const char*s){int n=0,sign=1;if(*s=='-'){s++;sign=-1;}while(*s>='0'&&*s<='9')n=n*10+*s++-'0';return n*sign;}
static void itoa(char*d,int v){char b[16];unsigned n=v;int i=0;if(v<0){*d++='-';n=0u-n;}do{b[i++]='0'+n%10;n/=10;}while(n);while(i)*d++=b[--i];*d=0;}
