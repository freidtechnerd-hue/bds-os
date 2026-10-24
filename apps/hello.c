#include "common.h"
int app_main(BdsApi*a,int argc,char**argv){a->print("Hello from native " BDS_ARCH_NAME " .RUN machine code!\n");return 0;}
