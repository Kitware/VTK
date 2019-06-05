/* double-precision floating-point traits */

#define Scalar double                      /* floating-point type */
#define Int int64                          /* corresponding signed integer type */
#define UInt uint64                        /* corresponding unsigned integer type */
#define EBITS 11                           /* number of exponent bits */
#define PBITS 6                            /* number of bits needed to encode precision */
#define NBMASK UINT64C(0xaaaaaaaaaaaaaaaa) /* negabinary mask */
#define TCMASK UINT64C(0x7fffffffffffffff) /* two's complement mask */

#define FABS(x) fabs(x)
#define FREXP(x, e) frexp(x, e)
#define LDEXP(x, e) ldexp(x, e)
