#ifndef BDS_LWIP_CC_H
#define BDS_LWIP_CC_H
#include "os.h"
#include <stddef.h>
#define LWIP_NO_STDINT_H 1
#define LWIP_NO_INTTYPES_H 1
#define LWIP_NO_CTYPE_H 1
#define LWIP_NO_LIMITS_H 1
#define INT_MAX 2147483647
#define UINT_MAX 4294967295u
typedef u8 u8_t;typedef signed char s8_t;
typedef u16 u16_t;typedef short s16_t;
typedef u32 u32_t;typedef int s32_t;typedef u32 mem_ptr_t;
#define U16_F "u"
#define S16_F "d"
#define X16_F "x"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"
#define X8_F "02x"
#define SZT_F "u"
#define BYTE_ORDER 1234
void net_assert(const char*);
u32 net_random(void);
#define LWIP_PLATFORM_DIAG(x) ((void)0)
#define LWIP_PLATFORM_ASSERT(x) net_assert(x)
#define LWIP_RAND() net_random()
#endif
