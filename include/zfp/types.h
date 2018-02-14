#ifndef ZFP_TYPES_H
#define ZFP_TYPES_H

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

#if __STDC_VERSION__ >= 199901L
  /* C99: use standard integer types */
  #include <stdint.h>
  #define INT64C(x) INT64_C(x)
  #define UINT64C(x) UINT64_C(x)
  typedef int8_t int8;
  typedef uint8_t uint8;
  typedef int16_t int16;
  typedef uint16_t uint16;
  typedef int32_t int32;
  typedef uint32_t uint32;
  typedef int64_t int64;
  typedef uint64_t uint64;
#else
  /* C89: assume common integer types */
  typedef signed char int8;
  typedef unsigned char uint8;
  typedef signed short int16;
  typedef unsigned short uint16;

  /* assume 32-bit integers (LP64, LLP64) */
  typedef signed int int32;
  typedef unsigned int uint32;

  /* determine 64-bit data model */
  #if defined(_WIN32) || defined(_WIN64)
    /* assume ILP32 or LLP64 (MSVC, MinGW) */
    #define ZFP_LLP64 1
  #else
    /* assume LP64 (Linux, macOS, ...) */
    #define ZFP_LP64 1
  #endif

  /* concatenation for literal suffixes */
  #define _zfp_cat_(x, y) x ## y
  #define _zfp_cat(x, y) _zfp_cat_(x, y)

  /* signed 64-bit integers */
  #if defined(ZFP_INT64) && defined(ZFP_INT64_SUFFIX)
    #define INT64C(x) _zfp_cat(x, ZFP_INT64_SUFFIX)
    typedef ZFP_INT64 int64;
  #elif ZFP_LP64
    #define INT64C(x) x ## l
    typedef signed long int64;
  #elif ZFP_LLP64
    #define INT64C(x) x ## ll
    typedef signed long long int64;
  #else
    #error "unknown 64-bit signed integer type"
  #endif

  /* unsigned 64-bit integers */
  #if defined(ZFP_UINT64) && defined(ZFP_UINT64_SUFFIX)
    #define UINT64C(x) _zfp_cat(x, ZFP_UINT64_SUFFIX)
    typedef ZFP_UINT64 uint64;
  #elif ZFP_LP64
    #define UINT64C(x) x ## ul
    typedef unsigned long uint64;
  #elif ZFP_LLP64
    #define UINT64C(x) x ## ull
    typedef unsigned long long uint64;
  #else
    #error "unknown 64-bit unsigned integer type"
  #endif
#endif

#endif
