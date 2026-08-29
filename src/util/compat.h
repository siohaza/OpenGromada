#ifndef COMPAT_H
#define COMPAT_H

#if defined(__MINGW32__) || (defined(_MSC_VER) && _MSC_VER >= 1100)
#define COMPAT_MODE
#endif

#pragma warning(disable : 4786)

#define MSVC420_VERSION 1020

#if __cplusplus < 201103L
#define override
#define static_assert(expr, msg)
#endif

#endif
