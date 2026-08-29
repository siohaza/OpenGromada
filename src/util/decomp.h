#ifndef DECOMP_H
#define DECOMP_H

#ifndef NDEBUG

#undef ENABLE_DECOMP_ASSERTS
#endif

#if defined(ENABLE_DECOMP_ASSERTS)
#define DECOMP_STATIC_ASSERT(V)                                                                                        \
	namespace                                                                                                          \
	{                                                                                                                  \
	typedef int foo[(V) ? 1 : -1];                                                                                     \
	}
#define DECOMP_SIZE_ASSERT(T, S) DECOMP_STATIC_ASSERT(sizeof(T) == S)
#else
#define DECOMP_STATIC_ASSERT(V)
#define DECOMP_SIZE_ASSERT(T, S)
#endif

#ifndef sizeOfArray
#define sizeOfArray(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

#ifdef BETA10
#define assertIfBeta10(A) assert(A)
#else
#define assertIfBeta10(A)
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
