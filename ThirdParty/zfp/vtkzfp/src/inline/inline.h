#ifndef INLINE_H
#define INLINE_H

#ifndef inline_
  #if __STDC_VERSION__ >= 199901L
    #define inline_ static inline
  #else
    #define inline_ static
  #endif
#endif

#endif
