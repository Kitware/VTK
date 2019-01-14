#ifndef DIY_CONSTANTS_H
#define DIY_CONSTANTS_H

// Default DIY_MAX_DIM to 4, unless provided by the user
// (used for static min/max size in various Bounds)
#ifndef DIY_MAX_DIM
#define DIY_MAX_DIM 4
#endif

enum
{
  DIY_X0 = 0x01, /* minimum-side x (left) neighbor */
  DIY_X1 = 0x02, /* maximum-side x (right) neighbor */
  DIY_Y0 = 0x04, /* minimum-side y (bottom) neighbor */
  DIY_Y1 = 0x08, /* maximum-side y (top) neighbor */
  DIY_Z0 = 0x10, /* minimum-side z (back) neighbor */
  DIY_Z1 = 0x20, /* maximum-side z (front)neighbor */
  DIY_T0 = 0x40, /* minimum-side t (earlier) neighbor */
  DIY_T1 = 0x80  /* maximum-side t (later) neighbor */
};

#define DIY_UNUSED(expr) do { (void)(expr); } while (0)

#endif
