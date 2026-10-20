#include "bds.h"
#include <cpuid.h>
extern int player_main(BdsApi*,int,char**);
int __attribute__((section(".text.app_main"),force_align_arg_pointer)) app_main(BdsApi*a,int argc,char**argv){
    unsigned x,b,c,d,cr4;
    if(a->version<6){a->print("Media Player requires BDS API 6.\n");return 1;}
    if(!__get_cpuid(1,&x,&b,&c,&d)||(d&(7u<<24))!=(7u<<24)){a->print("Media Player requires an SSE2 CPU.\n");return 1;}
    __asm__("mov %%cr4,%0":"=r"(cr4));if(!(cr4&512)){a->print("Media Player requires SSE kernel support.\n");return 1;}
    return player_main(a,argc,argv);
}
