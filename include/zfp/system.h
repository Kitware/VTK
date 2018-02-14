#ifndef ZFP_SYSTEM_H
#define ZFP_SYSTEM_H

#if __STDC_VERSION__ >= 199901L
  #define restrict_ restrict
#else
  #define restrict_
#endif

/* macros for exporting and importing symbols */
#ifdef _MSC_VER
  #define export_ __declspec(dllexport)
  /* export (import) symbols when ZFP_SOURCE is (is not) defined */
  #ifdef ZFP_SOURCE
    #ifdef __cplusplus
      #define extern_ extern "C" __declspec(dllexport)
    #else
      #define extern_ extern     __declspec(dllexport)
    #endif
  #else
    #ifdef __cplusplus
      #define extern_ extern "C" __declspec(dllimport)
    #else
      #define extern_ extern     __declspec(dllimport)
    #endif
  #endif
#else /* !_MSC_VER */
  #define export_
  #ifdef __cplusplus
    #define extern_ extern "C"
  #else
    #define extern_ extern
  #endif
#endif

#ifdef __GNUC__
  /* L1 cache line size for alignment purposes */
  #ifndef ZFP_CACHE_LINE_SIZE
    #define ZFP_CACHE_LINE_SIZE 0x100
  #endif
  #define align_(n) __attribute__((aligned(n)))
  #define cache_align_(x) x align_(ZFP_CACHE_LINE_SIZE)
#else
  #define cache_align_(x) x
#endif

#endif
