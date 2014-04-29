#ifndef __stdint_h__
#define __stdint_h__

/* Copy to include
#include "wstdint.h"
*/


#ifdef __MACOS__
	#include <stddef.h>
	#include </usr/include/stdint.h>  // Mac has this file in /usr/includez
#endif
#ifdef __linux__
	#if ! defined(__STDC_LIMIT_MACROS)
		#define __STDC_LIMIT_MACROS  
	#endif
	
	#include <stddef.h>
	#include </usr/include/stdint.h>
#endif

#if (defined (_WINDOWS) || defined(WIN32) || defined(WIN64))
#if (_MSC_VER > 1600) || defined(__MINGW64__)
    // Taken from MSDN official page:
    // In Visual Studio 2010 _MSC_VER  is defined as 1600, In Visual Studio 2012 _MSC_VER  is defined as 1700.
    #include <stdint.h>
#else
#ifndef _STDINT_H
    #define _STDINT_H // this will prevent Altura's CGBase.h from defining int32_t
#endif
/*
 * ISO C 99 <stdint.h> for platforms that lack it.
 * <http://www.opengroup.org/onlinepubs/007904975/basedefs/stdint.h.html>
 */

/* Get wchar_t, WCHAR_MIN, WCHAR_MAX.  */
#include <stddef.h>
/* Get CHAR_BIT, LONG_MIN, LONG_MAX, ULONG_MAX.  */
#include <limits.h>

/* Get those types that are already defined in other system include files.  */
#if defined(__FreeBSD__)
# include <sys/inttypes.h>
#endif

#if defined(__sun) && HAVE_SYS_INTTYPES_H
# include <sys/inttypes.h>
  /* Solaris 7 <sys/inttypes.h> has the types except the *_fast*_t types, and
     the macros except for *_FAST*_*, INTPTR_MIN, PTRDIFF_MIN, PTRDIFF_MAX.
     But note that <sys/int_types.h> contains only the type definitions!  */
# define HAVE_SYSTEM_INTTYPES
#endif
#if (defined(__hpux) || defined(_AIX)) && HAVE_INTTYPES_H
# include <inttypes.h>
  /* HP-UX 10 <inttypes.h> has nearly everything, except UINT_LEAST8_MAX,
     UINT_FAST8_MAX, PTRDIFF_MIN, PTRDIFF_MAX.  */
  /* AIX 4 <inttypes.h> has nearly everything, except INTPTR_MIN, INTPTR_MAX,
     UINTPTR_MAX, PTRDIFF_MIN, PTRDIFF_MAX.  */
# define HAVE_SYSTEM_INTTYPES
#endif
#if !(defined(UNIX_CYGWIN32) && defined(__BIT_TYPES_DEFINED__))
# define NEED_SIGNED_INT_TYPES
#endif

#if !defined(HAVE_SYSTEM_INTTYPES)

/* 7.18.1.1. Exact-width integer types */
#if !defined(__FreeBSD__)

#if defined(_MSC_VER)
typedef          __int8  int8_t;
typedef unsigned __int8  uint8_t;
typedef          __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef          __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef          __int64 int64_t;
typedef unsigned __int64 uint64_t;


#else // _MSC_VER

#ifdef NEED_SIGNED_INT_TYPES
typedef signed char    int8_t;
#endif
typedef unsigned char  uint8_t;

#ifdef NEED_SIGNED_INT_TYPES
typedef short          int16_t;
#endif
typedef unsigned short uint16_t;

#ifdef NEED_SIGNED_INT_TYPES
typedef int            int32_t;
#endif
typedef unsigned int   uint32_t;

#if 0
#ifdef NEED_SIGNED_INT_TYPES
typedef long           int64_t;
#endif
typedef unsigned long  uint64_t;
#elif 0
#ifdef NEED_SIGNED_INT_TYPES
typedef long long          int64_t;
#endif
typedef unsigned long long uint64_t;
#endif

#endif // _MSC_VER

#endif /* !FreeBSD */

/* 7.18.1.2. Minimum-width integer types */

typedef int8_t   int_least8_t;
typedef uint8_t  uint_least8_t;
typedef int16_t  int_least16_t;
typedef uint16_t uint_least16_t;
#if !defined(kAlturaAlreadyDefinesInt32)
typedef int32_t  int_least32_t;
#endif
typedef uint32_t uint_least32_t;
typedef int64_t  int_least64_t;
typedef uint64_t uint_least64_t;


/* 7.18.1.3. Fastest minimum-width integer types */

typedef int32_t  int_fast8_t;
typedef uint32_t uint_fast8_t;
typedef int32_t  int_fast16_t;
typedef uint32_t uint_fast16_t;
typedef int32_t  int_fast32_t;
typedef uint32_t uint_fast32_t;
typedef int64_t  int_fast64_t;
typedef uint64_t uint_fast64_t;


/* 7.18.1.4. Integer types capable of holding object pointers */

#if !defined(__FreeBSD__)

/* On some platforms (like IRIX6 MIPS with -n32) sizeof(void*) < sizeof(long),
   but this doesn't matter here.  */
#if !defined(_INTPTR_T_DEFINED)
typedef long          intptr_t;
#define _INTPTR_T_DEFINED
#endif
#if !defined(_UINTPTR_T_DEFINED)
typedef unsigned long uintptr_t;
#define _UINTPTR_T_DEFINED
#endif

#endif /* !FreeBSD */

/* 7.18.1.5. Greatest-width integer types */


typedef int64_t  intmax_t;
typedef uint64_t uintmax_t;
#if 0 || 0
typedef int32_t  intmax_t;
typedef uint32_t uintmax_t;
#endif

/* 7.18.2. Limits of specified-width integer types */

//#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS)

/* 7.18.2.1. Limits of exact-width integer types */

#define INT8_MIN  -128
#define INT8_MAX   127
#define UINT8_MAX  255U
#define INT16_MIN  -32768
#define INT16_MAX   32767
#define UINT16_MAX  65535U
#define INT32_MIN   (~INT32_MAX)
#define INT32_MAX   2147483647
#define UINT32_MAX  4294967295U
#if 0
#define INT64_MIN   (~INT64_MIN)
#define INT64_MAX   9223372036854775807L
#define UINT64_MAX 18446744073709551615UL
#elif 0
#define INT64_MIN   (~INT64_MIN)
#define INT64_MAX   9223372036854775807LL
#define UINT64_MAX 18446744073709551615ULL
#endif

/* 7.18.2.2. Limits of minimum-width integer types */

#define INT_LEAST8_MIN INT8_MIN
#define INT_LEAST8_MAX INT8_MAX
#define UINT_LEAST8_MAX UINT8_MAX
#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST16_MAX INT16_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST32_MAX INT32_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#if 0 || 0
#define INT_LEAST64_MIN INT64_MIN
#define INT_LEAST64_MAX INT64_MAX
#define UINT_LEAST64_MAX UINT64_MAX
#endif

/* 7.18.2.3. Limits of fastest minimum-width integer types */

#define INT_FAST8_MIN INT32_MIN
#define INT_FAST8_MAX INT32_MAX
#define UINT_FAST8_MAX UINT32_MAX
#define INT_FAST16_MIN INT32_MIN
#define INT_FAST16_MAX INT32_MAX
#define UINT_FAST16_MAX UINT32_MAX
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST32_MAX INT32_MAX
#define UINT_FAST32_MAX UINT32_MAX
#if 0 || 0
#define INT_FAST64_MIN INT64_MIN
#define INT_FAST64_MAX INT64_MAX
#define UINT_FAST64_MAX UINT64_MAX
#endif

/* 7.18.2.4. Limits of integer types capable of holding object pointers */

#define INTPTR_MIN LONG_MIN
#define INTPTR_MAX LONG_MAX
#define UINTPTR_MAX ULONG_MAX

/* 7.18.2.5. Limits of greatest-width integer types */

#if 0 || 0
#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define UINTMAX_MAX UINT64_MAX
#else
#define INTMAX_MIN INT32_MIN
#define INTMAX_MAX INT32_MAX
#define UINTMAX_MAX UINT32_MAX
#endif

/* 7.18.3. Limits of other integer types */

#define PTRDIFF_MIN (~(ptrdiff_t)0 << (sizeof(ptrdiff_t)*CHAR_BIT-1))
#define PTRDIFF_MAX (~PTRDIFF_MIN)

/* This may be wrong...  */
#define SIG_ATOMIC_MIN 0
#define SIG_ATOMIC_MAX 127

//#define SIZE_MAX (~(size_t)0)

/* wchar_t limits already defined in <stddef.h>.  */
/* wint_t limits already defined in <wchar.h>.  */

//#endif

/* 7.18.4. Macros for integer constants */

#if !defined(__cplusplus) || defined(__STDC_CONSTANT_MACROS)

/* 7.18.4.1. Macros for minimum-width integer constants */

#ifdef INT8_C
#undef INT8_C
#endif
#define INT8_C(x) x

#ifdef UINT8_C
#undef UINT8_C
#endif
#define UINT8_C(x) x##U

#ifdef INT16_C
#undef INT16_C
#endif
#define INT16_C(x) x

#ifdef UINT16_C
#undef UINT16_C
#endif
#define UINT16_C(x) x##U

#ifdef INT32_C
#undef INT32_C
#endif
#define INT32_C(x) x

#ifdef UINT32_C
#undef UINT32_C
#endif
#define UINT32_C(x) x##U

// INT64_C and UINT64_C definitions
#ifdef INT64_C
#undef INT64_C
#endif
#ifdef UINT64_C
#undef UINT64_C
#endif
#if 0
#define INT64_C(x) x##L
#define UINT64_C(x) x##UL
#elif 0
#define INT64_C(x) x##LL
#define UINT64_C(x) x##ULL
#endif // #if 0

/* 7.18.4.2. Macros for greatest-width integer constants */

// INTMAX_C and UINTMAX_C definitions
#ifdef INTMAX_C
#undef INTMAX_C
#endif
#ifdef UINTMAX_C
#undef UINTMAX_C
#endif

#if 0
#define INTMAX_C(x) x##L
#define UINTMAX_C(x) x##UL
#elif 0
#define INTMAX_C(x) x##LL
#define UINTMAX_C(x) x##ULL
#else
#define INTMAX_C(x) x
#define UINTMAX_C(x) x##U
#endif

#endif

#endif  /* !HAVE_SYSTEM_INTTYPES */

#endif /* (_MSC_VER < 1400) */

#endif /* #ifdef _WINDOWS */

#endif /* __stdint_h__ */
