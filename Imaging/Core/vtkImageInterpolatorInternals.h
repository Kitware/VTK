// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInterpolatorInternals
 * @brief   internals for vtkImageInterpolator
 */

#ifndef vtkImageInterpolatorInternals_h
#define vtkImageInterpolatorInternals_h

#include "vtkAbstractImageInterpolator.h"
#include "vtkEndian.h"
#include "vtkMath.h"

// The interpolator info struct
VTK_ABI_NAMESPACE_BEGIN
struct vtkInterpolationInfo
{
  const void* Pointer;
  int Extent[6];
  vtkIdType Increments[3];
  int ScalarType;
  int NumberOfComponents;
  vtkImageBorderMode BorderMode;
  int InterpolationMode;
  void* ExtraInfo;

  vtkDataArray* Array;
  vtkIdType Index;
};

// The interpolation weights struct
struct vtkInterpolationWeights : public vtkInterpolationInfo
{
  vtkIdType* Positions[3];
  void* Weights[3];
  int WeightExtent[6];
  int KernelSize[3];
  int WeightType; // VTK_FLOAT or VTK_DOUBLE
  void* Workspace;
  int LastY;
  int LastZ;

  // partial copy constructor from superclass
  vtkInterpolationWeights(const vtkInterpolationInfo& info)
    : vtkInterpolationInfo(info)
    , Workspace(nullptr)
  {
  }
};

// The internal math functions for the interpolators
struct vtkInterpolationMath
{
  // floor with remainder (remainder can be double or float),
  // includes a small tolerance for values just under an integer
  template <class F>
  static int Floor(double x, F& f);

  // round function optimized for various architectures
  static int Round(double x);

  // border-handling functions for keeping index a with in bounds b, c
  static int Clamp(int a, int b, int c);
  static int Wrap(int a, int b, int c);
  static int Mirror(int a, int b, int c);
};

//--------------------------------------------------------------------------
// The 'floor' function is slow, so we want a faster replacement.
// The goal is to cast double to integer, but round down instead of
// rounding towards zero. In other words, we want -0.1 to become -1.

// The easiest way to do this is to add a large value (a bias)
// to the input of our 'fast floor' in order to ensure that the
// double that we cast to integer is positive. This ensures the
// cast will round the value down. After the cast, we can subtract
// the bias from the integer result.

// We choose a bias of 103079215104 because it has a special property
// with respect to ieee-754 double-precision floats.  It uses 37 bits
// of the 53 significant bits available, leaving 16 bits of precision
// after the radix.  And the same is true for any number in the range
// [-34359738368,34359738367] when added to this bias.  This is a
// very large range, 16 times the range of a 32-bit int.  Essentially,
// this bias allows us to use the mantissa of a 'double' as a 52-bit
// (36.16) fixed-point value.  Hence, we can use our floating-point
// hardware for fixed-point math, with float-to-fixed and vice-versa
// conversions achieved by simply by adding or subtracting the bias.
// See http://www.stereopsis.com/FPU.html for further explanation.

// The advantage of fixed (absolute) precision over float (relative)
// precision is that when we do math on a coordinate (x,y,z) in the
// image, the available precision will be the same regardless of
// whether x, y, z are close to (0,0,0) or whether they are far away.
// This protects against a common problem in computer graphics where
// there is lots of available precision near the origin, but less
// precision far from the origin.  Instead of relying on relative
// precision, we have enforced the use of fixed precision.  As a
// trade-off, we are limited to the range [-34359738368,34359738367].

// The value 2^-17 (around 7.6e-6) is exactly half the value of the
// 16th bit past the decimal, so it is a useful tolerance to apply in
// our calculations.  For our 'fast floor', floating-point values
// that are within this tolerance from the closest integer will always
// be rounded to the integer, even when the value is less than the
// integer.  Values further than this tolerance from an integer will
// always be rounded down.

#define VTK_INTERPOLATE_FLOOR_TOL 7.62939453125e-06

// A fast replacement for 'floor' that provides fixed precision:
template <class F>
inline int vtkInterpolationMath::Floor(double x, F& f)
{
#if VTK_SIZEOF_VOID_P >= 8
  // add the bias and then subtract it to achieve the desired result
  x += (103079215104.0 + VTK_INTERPOLATE_FLOOR_TOL);
  long long i = static_cast<long long>(x);
  f = static_cast<F>(x - i);
  return static_cast<int>(i - 103079215104LL);
#elif !defined VTK_WORDS_BIGENDIAN
  // same as above, but avoid doing any 64-bit integer arithmetic
  union
  {
    double d;
    unsigned short s[4];
    unsigned int i[2];
  } dual;
  dual.d = x + 103079215104.0;        // (2**(52-16))*1.5
  f = dual.s[0] * 0.0000152587890625; // 2**(-16)
  return static_cast<int>((dual.i[1] << 16) | ((dual.i[0]) >> 16));
#else
  // and again for big-endian architectures
  union
  {
    double d;
    unsigned short s[4];
    unsigned int i[2];
  } dual;
  dual.d = x + 103079215104.0;        // (2**(52-16))*1.5
  f = dual.s[3] * 0.0000152587890625; // 2**(-16)
  return static_cast<int>((dual.i[0] << 16) | ((dual.i[1]) >> 16));
#endif
}

inline int vtkInterpolationMath::Round(double x)
{
#if VTK_SIZEOF_VOID_P >= 8
  // add the bias and then subtract it to achieve the desired result
  x += (103079215104.5 + VTK_INTERPOLATE_FLOOR_TOL);
  long long i = static_cast<long long>(x);
  return static_cast<int>(i - 103079215104LL);
#elif !defined VTK_WORDS_BIGENDIAN
  // same as above, but avoid doing any 64-bit integer arithmetic
  union
  {
    double d;
    unsigned int i[2];
  } dual;
  dual.d = x + 103079215104.5; // (2**(52-16))*1.5
  return static_cast<int>((dual.i[1] << 16) | ((dual.i[0]) >> 16));
#else
  // and again for big-endian architectures
  union
  {
    double d;
    unsigned int i[2];
  } dual;
  dual.d = x + 103079215104.5; // (2**(52-16))*1.5
  return static_cast<int>((dual.i[0] << 16) | ((dual.i[1]) >> 16));
#endif
}

//----------------------------------------------------------------------------
// Perform a clamp to limit an index to [b, c] and subtract b.

inline int vtkInterpolationMath::Clamp(int a, int b, int c)
{
  a = (a <= c ? a : c);
  a -= b;
  a = (a >= 0 ? a : 0);
  return a;
}

//----------------------------------------------------------------------------
// Perform a wrap to limit an index to [b, c] and subtract b.

inline int vtkInterpolationMath::Wrap(int a, int b, int c)
{
  int range = c - b + 1;
  a -= b;
  a %= range;
  // required for some % implementations
  a = (a >= 0 ? a : a + range);
  return a;
}

//----------------------------------------------------------------------------
// Perform a mirror to limit an index to [b, c] and subtract b.

inline int vtkInterpolationMath::Mirror(int a, int b, int c)
{
#ifndef VTK_IMAGE_BORDER_LEGACY_MIRROR
  int range = c - b;
  int ifzero = (range == 0);
  int range2 = 2 * range + ifzero;
  a -= b;
  a = (a >= 0 ? a : -a);
  a %= range2;
  a = (a <= range ? a : range2 - a);
  return a;
#else
  int range = c - b + 1;
  int range2 = 2 * range;
  a -= b;
  a = (a >= 0 ? a : -a - 1);
  a %= range2;
  a = (a < range ? a : range2 - a - 1);
  return a;
#endif
}

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkImageInterpolatorInternals.h
