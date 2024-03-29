#ifndef UTILS
#define UTILS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__STDC__)
# define C89
# if defined(__STDC_VERSION__)
#  define C90
#  if (__STDC_VERSION__ >= 199901L) && ! defined C99
#   define C99
#  endif
# endif
#endif

#ifdef __STRICT_ANSI__
#include <sys/types.h>
#endif

#ifdef C99
# include <stdbool.h>
#else
 typedef enum { false = 0, true = 1 } bool;
#endif

void usage(char *progname, FILE *outstream);

FILE *xfopen(const char *path, const char *mode);
void *xrealloc(void *ptr, size_t size);
void *xmalloc(size_t size);
char *xstrdup(const char *str);

typedef enum {
	NUM_EXPECTED,
	SYM_EXPECTED,
	ADDR_EXPECTED,
	SYM_TOLONG,
	TOO_MANY_DEF_IN_MODULE,
	TOO_MANY_USE_IN_MODULE,
	TOO_MANY_SYMBOLS,
	TOO_MANY_INSTR
} Error_Codes;

void __parseerror(int errcode, int linenum, int lineoffset);

extern const char *current_workfile;

#endif
