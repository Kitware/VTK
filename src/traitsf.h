/* single-precision floating-point traits */

#define Scalar float       /* floating-point type */
#define Int int32          /* corresponding signed integer type */
#define UInt uint32        /* corresponding unsigned integer type */
#define EBITS 8            /* number of exponent bits */
#define PBITS 5            /* number of bits needed to encode precision */
#define NBMASK 0xaaaaaaaau /* negabinary mask */
#define TCMASK 0x7fffffffu /* two's complement mask */

#if __STDC_VERSION__ >= 199901L
  #define FABS(x)     fabsf(x)
  #define FREXP(x, e) frexpf(x, e)
  #define LDEXP(x, e) ldexpf(x, e)
#else
  #define FABS(x)     (float)fabs(x)
  #define FREXP(x, e) (void)frexp(x, e)
  #define LDEXP(x, e) (float)ldexp(x, e)
#endif
