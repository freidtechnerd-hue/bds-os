#ifndef BDS_LIBC_H
#define BDS_LIBC_H
#include <stdarg.h>
typedef unsigned int size_t;typedef int ptrdiff_t;
typedef unsigned char uint8_t;typedef signed char int8_t;
typedef unsigned short uint16_t;typedef short int16_t;
typedef unsigned int uint32_t;typedef int int32_t;
typedef unsigned long long uint64_t;typedef long long int64_t;
typedef unsigned int uintptr_t;typedef int intptr_t;
#ifndef NULL
#define NULL ((void*)0)
#endif
#define INT_MAX 2147483647
#define INT_MIN (-INT_MAX-1)
#define UINT_MAX 4294967295u
#define SHRT_MAX 32767
#define SHRT_MIN (-32768)
#define USHRT_MAX 65535
#define LONG_MAX INT_MAX
#define CHAR_BIT 8
#define SIZE_MAX UINT_MAX
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define EISDIR 21
#define ENOENT 2
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
extern int errno;
typedef struct BdsFile{int id,mode,error;size_t pos,size,capacity;char path[256];unsigned char*data;}FILE;
extern FILE *stdin,*stdout,*stderr;
void *malloc(size_t);void free(void*);void *calloc(size_t,size_t);void *realloc(void*,size_t);
void *memcpy(void*,const void*,size_t);void *memmove(void*,const void*,size_t);void *memset(void*,int,size_t);int memcmp(const void*,const void*,size_t);
size_t strlen(const char*);char *strcpy(char*,const char*);char *strncpy(char*,const char*,size_t);char *strcat(char*,const char*);
int strcmp(const char*,const char*);int strncmp(const char*,const char*,size_t);int strcasecmp(const char*,const char*);int strncasecmp(const char*,const char*,size_t);
char *strchr(const char*,int);char *strrchr(const char*,int);char *strstr(const char*,const char*);char *strdup(const char*);
int atoi(const char*);double atof(const char*);double strtod(const char*,char**);long strtol(const char*,char**,int);int abs(int);int toupper(int);int tolower(int);int isspace(int);int isprint(int);int isdigit(int);int isalpha(int);
int isalnum(int);
char *getenv(const char*);int system(const char*);void exit(int) __attribute__((noreturn));void abort(void) __attribute__((noreturn));
FILE *fopen(const char*,const char*);int fclose(FILE*);size_t fread(void*,size_t,size_t,FILE*);size_t fwrite(const void*,size_t,size_t,FILE*);int fseek(FILE*,long,int);long ftell(FILE*);int feof(FILE*);int fflush(FILE*);int ferror(FILE*);int fgetc(FILE*);int fputc(int,FILE*);char *fgets(char*,int,FILE*);
int vsnprintf(char*,size_t,const char*,va_list);int snprintf(char*,size_t,const char*,...);int sprintf(char*,const char*,...);int printf(const char*,...);int fprintf(FILE*,const char*,...);int vfprintf(FILE*,const char*,va_list);int puts(const char*);int putchar(int);
int sscanf(const char*,const char*,...);int fscanf(FILE*,const char*,...);int remove(const char*);int rename(const char*,const char*);int mkdir(const char*,int);
double fabs(double);double atan(double);double sin(double);
void bds_assert(const char*,int);
#define assert(x) ((x)?(void)0:bds_assert(__FILE__,__LINE__))
#endif
