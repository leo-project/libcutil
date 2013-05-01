#ifndef _LIBCUTIL_H_
#define _LIBCUTIL_H_

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <math.h>

typedef signed char             int8;
typedef unsigned char           uint8;
typedef signed short            int16;
typedef unsigned short          uint16;
typedef signed int              int32;
typedef unsigned int            uint32;
typedef signed long long int    int64;
typedef unsigned long long int  uint64;
typedef float                   float32;
typedef double                  float64;

#if defined(__ia64) || defined(__x86_64) || defined(__amd64)
typedef uint64          uintptr;
typedef int64           intptr;
typedef int64           intgo;
typedef uint64          uintgo;
#else
typedef uint32          uintptr;
typedef int32           intptr;
typedef int32           intgo;
typedef uint32          uintgo;
#endif

typedef uint8_t byte;

#ifdef DEBUG
#define dprintf(format, args...)  printf (format , ## args)
#else
#define dprintf(format, args...)
#endif

#include <lcu_string.h>
#include <lcu_queue.h>
#include <lcu_lru.h>
#include <lcu_slab.h>
#include <lcu_hashmap.h>
#include <lcu_cache.h>

#define LCU_OK  0
#define LCU_NG -1

#define nelem(x)        (sizeof(x)/sizeof((x)[0]))
#define nil             ((void*)0)
#ifndef offsetof
    #define offsetof(s,m)   (uint32)(&(((s*)0)->m))
#endif
#define ROUND(x, n)     (((x)+(n)-1)&~((n)-1))

#endif
