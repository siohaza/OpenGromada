#ifndef DECOMP_H
#define DECOMP_H

#ifndef sizeOfArray
#define sizeOfArray(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

// Enables printf format checking.
#if defined(__GNUC__)
#define DECOMP_PRINTF(fmt, first) __attribute__((format(printf, fmt, first)))
#else
#define DECOMP_PRINTF(fmt, first)
#endif

typedef unsigned char undefined;
typedef unsigned short undefined2;
typedef unsigned int undefined4;

#if defined(_WIN64)
typedef __int64 decomp_intptr;
#elif defined(__LP64__)
typedef long decomp_intptr;
#else
typedef int decomp_intptr;
#endif

#endif
