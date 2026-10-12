#include "common.h"
int app_main(BdsApi*a,int argc,char**argv){char s[16];a->print("Native system utility; BDS ABI version ");itoa(s,a->version);a->print(s);a->print("\nDisplay width: ");itoa(s,a->width);a->print(s);a->print("\nUptime milliseconds: ");itoa(s,a->ticks());a->print(s);a->print("\n");return 0;}
