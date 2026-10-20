#include "libc.h"
/* Only liba52 table construction needs these two elementary functions. */
double cos(double x){return sin(x+1.57079632679489661923);}
double sqrt(double x){double y;__asm__("sqrtsd %1,%0":"=x"(y):"x"(x));return y;}
#define PL_MPEG_IMPLEMENTATION
#define PLM_NO_STDIO
#include "pl_mpeg.h"
