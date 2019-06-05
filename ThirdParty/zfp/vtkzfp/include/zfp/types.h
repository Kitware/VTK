#ifndef ZFP_TYPES_H
#define ZFP_TYPES_H

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

#if __STDC_VERSION__ >= 199901L
  /* C99: use standard integer types */
  #include <stdint.h>
  #include <inttypes.h>
  #define INT64C(x) INT64_C(x)
  #define UINT64C(x) UINT64_C(x)
  #define INT64PRId PRId64
  #define INT64PRIi PRIi64
  #define UINT64PRIo PRIo64
  #define UINT64PRIu PRIu64
  #define UINT64PRIx PRIx64
  #define INT64SCNd SCNd64
  #define INT64SCNi SCNi64
  #define UINT64SCNo SCNo64
  #define UINT64SCNu SCNu64
  #define UINT64SCNx SCNx64
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
    #define INT64PRId #ZFP_INT64_SUFFIX "d"
    #define INT64PRIi #ZFP_INT64_SUFFIX "i"
    typedef ZFP_INT64 int64;
  #elif ZFP_LP64
    #define INT64C(x) x ## l
    #define INT64PRId "ld"
    #define INT64PRIi "li"
    typedef signed long int64;
  #elif ZFP_LLP64
    #define INT64C(x) x ## ll
    #define INT64PRId "lld"
    #define INT64PRIi "lli"
    typedef signed long long int64;
  #else
    #error "unknown 64-bit signed integer type"
  #endif
  #define INT64SCNd INT64PRId
  #define INT64SCNi INT64PRIi

  /* unsigned 64-bit integers */
  #if defined(ZFP_UINT64) && defined(ZFP_UINT64_SUFFIX)
    #define UINT64C(x) _zfp_cat(x, ZFP_UINT64_SUFFIX)
    #ifdef ZFP_INT64_SUFFIX
      #define UINT64PRIo #ZFP_INT64_SUFFIX "o"
      #define UINT64PRIu #ZFP_INT64_SUFFIX "u"
      #define UINT64PRIx #ZFP_INT64_SUFFIX "x"
    #endif
    typedef ZFP_UINT64 uint64;
  #elif ZFP_LP64
    #define UINT64C(x) x ## ul
    #define UINT64PRIo "lo"
    #define UINT64PRIu "lu"
    #define UINT64PRIx "lx"
    typedef unsigned long uint64;
  #elif ZFP_LLP64
    #define UINT64C(x) x ## ull
    #define UINT64PRIo "llo"
    #define UINT64PRIu "llu"
    #define UINT64PRIx "llx"
    typedef unsigned long long uint64;
  #else
    #error "unknown 64-bit unsigned integer type"
  #endif
  #define UINT64SCNo UINT64PRIo
  #define UINT64SCNu UINT64PRIu
  #define UINT64SCNx UINT64PRIx
#endif

#endif
