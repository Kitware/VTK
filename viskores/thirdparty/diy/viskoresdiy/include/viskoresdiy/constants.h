#ifndef VISKORESDIY_CONSTANTS_H
#define VISKORESDIY_CONSTANTS_H

// Default VISKORESDIY_MAX_DIM to 4, unless provided by the user
// (used for static min/max size in various Bounds)
#ifndef VISKORESDIY_MAX_DIM
#define VISKORESDIY_MAX_DIM 4
#endif

enum
{
  VISKORESDIY_X0 = 0x01, /* minimum-side x (left) neighbor */
  VISKORESDIY_X1 = 0x02, /* maximum-side x (right) neighbor */
  VISKORESDIY_Y0 = 0x04, /* minimum-side y (bottom) neighbor */
  VISKORESDIY_Y1 = 0x08, /* maximum-side y (top) neighbor */
  VISKORESDIY_Z0 = 0x10, /* minimum-side z (back) neighbor */
  VISKORESDIY_Z1 = 0x20, /* maximum-side z (front)neighbor */
  VISKORESDIY_T0 = 0x40, /* minimum-side t (earlier) neighbor */
  VISKORESDIY_T1 = 0x80  /* maximum-side t (later) neighbor */
};

#define VISKORESDIY_UNUSED(expr) do { (void)(expr); } while (0)

// From https://stackoverflow.com/a/21265197/44738
#if defined(__cplusplus) && (__cplusplus >= 201402L)
#  define DEPRECATED(msg) [[deprecated(#msg)]]
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define DEPRECATED(msg) __attribute__((deprecated(#msg)))
#  elif defined(_MSC_VER)
#    define DEPRECATED(msg) __declspec(deprecated(#msg))
#  else
#    pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#    define DEPRECATED(msg)
#  endif
#endif


#endif
