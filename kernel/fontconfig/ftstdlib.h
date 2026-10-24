#include "../../third_party/freetype/include/freetype/config/ftstdlib.h"
#undef ft_jmp_buf
#undef ft_setjmp
#undef ft_longjmp
typedef void* BdxFontJump[5];
#define ft_jmp_buf BdxFontJump
#define ft_setjmp(b) __builtin_setjmp((void*)b)
#define ft_longjmp(b,v) __builtin_longjmp((void*)b,1)
#undef ft_strtol
#undef ft_getenv
#undef ft_qsort
#define ft_strtol bdx_font_strtol
#define ft_getenv bdx_font_getenv
#define ft_qsort bdx_font_qsort
long bdx_font_strtol(const char*,char**,int);
char*bdx_font_getenv(const char*);
void bdx_font_qsort(void*,size_t,size_t,int(*)(const void*,const void*));
