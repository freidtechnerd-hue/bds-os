#include "common.h"
int app_main(BdsApi*a,int argc,char**argv){
    if(argc!=4){a->print("Usage: RUN CALC.RUN number +|-|*|/ number\n");return 1;}
    int x=num(argv[1]),y=num(argv[3]),r=0;char op=*argv[2];
    if(op=='+')r=x+y;else if(op=='-')r=x-y;else if(op=='*')r=x*y;else if(op=='/'&&y)r=x/y;else{a->print("Invalid operator or divide by zero.\n");return 1;}
    char s[16];itoa(s,r);a->print("Result: ");a->print(s);a->print("\n");return 0;
}
