/* Check before entering any SSE2-compiled code, including its prologue. */
#include "bds.h"
#include <cpuid.h>
extern int cube_main(BdsApi*,int,char**);
int __attribute__((section(".text.app_main"),force_align_arg_pointer)) app_main(BdsApi* api,int argc,char** argv){
    unsigned a,b,c,d,cr4;
    if(api->version<5){api->print("ClassiCube requires BDS API 5.\n");return -1;}
    if(!__get_cpuid(1,&a,&b,&c,&d)||(d&(7u<<24))!=(7u<<24)){
        api->print("ClassiCube requires an SSE2-capable CPU.\n");return -1;
    }
    __asm__ volatile("mov %%cr4,%0":"=r"(cr4));
    if(!(cr4&0x200)){api->print("ClassiCube requires a kernel with SSE enabled.\n");return -1;}
    return cube_main(api,argc,argv);
}
