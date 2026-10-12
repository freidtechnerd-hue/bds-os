#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "elk.h"
size_t bds_js_number(double d,char*out,size_t n){return snprintf(out,n,"%.12g",d);}
static unsigned char memory[256*1024];
int main(void){
    const char*programs[]={"let x=0; for(let i=0;i<7;i++){x=x+i;} 'Result '+(x*2);","for(;;){}","let f=function(){return f();}; f();","1.1 === 1.9;"};
    for(int i=0;i<4;i++){struct js*j=js_create(memory,sizeof(memory));js_setmaxcss(j,8192);js_setbudget(j,20000);jsval_t result=js_eval(j,programs[i],strlen(programs[i]));const char*s=js_str(j,result);puts(s);if(i==0)assert(strstr(s,"Result 42"));if(i==1)assert(js_type(result)==JS_ERR&&strstr(s,"execution budget"));if(i==2)assert(js_type(result)==JS_ERR);if(i==3)assert(js_type(result)==JS_FALSE);}
}
