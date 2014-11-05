#ifndef _CONFIG_GUARD
#define _CONFIG_GUARD 1

#define STDC_HEADERS 1
#define HAVE_MEMCHR 1
#define HAVE_STRERROR 1
#define getpagesize() 0
#define ssize_t int
#define STDOUT_FILENO 1
#define VERSION "1.0"
#define HAVE_STRING_H 1
#define HAVE_DECL_STRERROR_R 1
#define HAVE_MBRTOWC 1
#define HAVE_MEMMOVE 1

typedef void  * DIR;
struct dirent {
	long d_ino;
	int d_off;
	unsigned short d_reclen;
	char d_name[1];
};

typedef unsigned __int64 uintmax_t;
#define _SIZE_T_DEFINED 1
typedef int size_t;

#define HAVE_DONE_WORKING_MALLOC_CHECK 1
#define HAVE_DONE_WORKING_REALLOC_CHECK 1
#define HAVE_DECL_STRTOUL 1
#define HAVE_DECL_STRTOULL 1

#define S_ISSOCK(a) 1
#define S_ISBLK(a) 1
#define S_ISCHR(a) 1
#define strcasecmp strcmp
#define on_exit(a) 1
#define quotearg_colon(a) ""
#endif