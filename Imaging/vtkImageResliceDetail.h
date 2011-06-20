/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResliceDetail.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This file is included by vtkImageReslice and vtkImageSlabReslice. It
// contains helper functions that are used by both classes. None of these
// methods are meant to be used by VTK users directly. A significant portion
// of this code originates from vtkImageReslice, which was developed by
// David Gobbi.

#ifndef __vtkImageResliceDetail_h
#define __vtkImageResliceDetail_h

#include "vtkImageResliceBase.h"
#include "vtkMath.h"
#include "vtkTemplateAliasMacro.h"
#include "vtkAbstractTransform.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include <limits.h>
#include <float.h>
#include <math.h>

// turn off 64-bit ints when templating over all types
# undef VTK_USE_INT64
# define VTK_USE_INT64 0
# undef VTK_USE_UINT64
# define VTK_USE_UINT64 0


//--------------------------------------------------------------------------
// DO NOT SET MAX KERNEL SIZE TO LARGER THAN 14
#define VTK_RESLICE_MAX_KERNEL_SIZE 14

// typedef for the floating point type used by the code
typedef double vtkImageResliceFloatingPointType;

//--------------------------------------------------------------------------
// The 'floor' function is slow, so we want to do an integer
// cast but keep the "floor" behavior of always rounding down,
// rather than truncating, i.e. we want -0.6 to become -1.
// The easiest way to do this is to add a large value in
// order to make the value "unsigned", then cast to int, and
// then subtract off the large value.

// On the old i386 architecture, even a cast to int is very
// expensive because it requires changing the rounding mode
// on the FPU.  So we use a bit-trick similar to the one
// described at http://www.stereopsis.com/FPU.html

#if defined ia64 || defined __ia64__ || defined _M_IA64
#define VTK_RESLICE_64BIT_FLOOR
#elif defined __ppc64__ || defined __x86_64__ || defined _M_X64
#define VTK_RESLICE_64BIT_FLOOR
#elif defined __ppc__ || defined sparc || defined mips
#define VTK_RESLICE_32BIT_FLOOR
#elif defined i386 || defined _M_IX86
#define VTK_RESLICE_I386_FLOOR
#endif

// We add a tolerance of 2^-17 (around 7.6e-6) so that float
// values that are just less than the closest integer are
// rounded up.  This adds robustness against rounding errors.

#define VTK_RESLICE_FLOOR_TOL 7.62939453125e-06


template<class F>
inline int vtkResliceFloor(double x, F &f)
{
#if defined VTK_RESLICE_64BIT_FLOOR
  x += (103079215104.0 + VTK_RESLICE_FLOOR_TOL);
#ifdef VTK_TYPE_USE___INT64
  __int64 i = static_cast<__int64>(x);
  f = x - i;
  return static_cast<int>(i - 103079215104i64);
#else
  long long i = static_cast<long long>(x);
  f = x - i;
  return static_cast<int>(i - 103079215104LL);
#endif
#elif defined VTK_RESLICE_32BIT_FLOOR
  x += (2147483648.0 + VTK_RESLICE_FLOOR_TOL);
  unsigned int i = static_cast<unsigned int>(x);
  f = x - i;
  return static_cast<int>(i - 2147483648U);
#elif defined VTK_RESLICE_I386_FLOOR
  union { double d; unsigned short s[4]; unsigned int i[2]; } dual;
  dual.d = x + 103079215104.0;  // (2**(52-16))*1.5
  f = dual.s[0]*0.0000152587890625; // 2**(-16)
  return static_cast<int>((dual.i[1]<<16)|((dual.i[0])>>16));
#else
  int i = vtkMath::Floor(x + VTK_RESLICE_FLOOR_TOL);
  f = x - i;
  return i;
#endif
}


inline int vtkResliceRound(double x)
{
#if defined VTK_RESLICE_64BIT_FLOOR
  x += (103079215104.5 + VTK_RESLICE_FLOOR_TOL);
#ifdef VTK_TYPE_USE___INT64
  __int64 i = static_cast<__int64>(x);
  return static_cast<int>(i - 103079215104i64);
#else
  long long i = static_cast<long long>(x);
  return static_cast<int>(i - 103079215104LL);
#endif
#elif defined VTK_RESLICE_32BIT_FLOOR
  x += (2147483648.5 + VTK_RESLICE_FLOOR_TOL);
  unsigned int i = static_cast<unsigned int>(x);
  return static_cast<int>(i - 2147483648U);
#elif defined VTK_RESLICE_I386_FLOOR
  union { double d; unsigned int i[2]; } dual;
  dual.d = x + 103079215104.5;  // (2**(52-16))*1.5
  return static_cast<int>((dual.i[1]<<16)|((dual.i[0])>>16));
#else
  return vtkMath::Floor(x + (0.5 + VTK_RESLICE_FLOOR_TOL));
#endif
}


//----------------------------------------------------------------------------
// constants for different boundary-handling modes

#define VTK_RESLICE_MODE_MASK 0x000f   // the interpolation modes
#define VTK_RESLICE_WRAP_MASK 0x0030   // the border handling modes
#define VTK_RESLICE_CLAMP     0x0010   // clamp to bounds of image
#define VTK_RESLICE_REPEAT    0x0020   // wrap to opposite side of image
#define VTK_RESLICE_MIRROR    0x0030   // mirror off of the boundary
#define VTK_RESLICE_N_MASK    0x0f00   // one less than kernel size
#define VTK_RESLICE_N_SHIFT   8        // position of size info
#define VTK_RESLICE_X_NEAREST 0x1000   // don't interpolate in x (hint)
#define VTK_RESLICE_Y_NEAREST 0x2000   // don't interpolate in y (hint)
#define VTK_RESLICE_Z_NEAREST 0x4000   // don't interpolate in z (hint)

static inline int vtkResliceGetMode(vtkImageResliceBase *self)
{
  int mode = self->GetInterpolationMode();

  if (self->GetMirror())
    {
    mode |= VTK_RESLICE_MIRROR;
    }
  else if (self->GetWrap())
    {
    mode |= VTK_RESLICE_REPEAT;
    }
  else
    {
    mode |= VTK_RESLICE_CLAMP;
    }

  // n is the kernel size subtract one, where the kernel size
  // must be an even number not larger than eight
  int n = 1;
  switch (mode & VTK_RESLICE_MODE_MASK)
    {
    case VTK_RESLICE_NEAREST:
      n = 1;
      break;
    case VTK_RESLICE_LINEAR:
    case VTK_RESLICE_RESERVED_2:
      n = 2;
      break;
    case VTK_RESLICE_CUBIC:
      n = 4;
      break;
    case VTK_RESLICE_LANCZOS:
    case VTK_RESLICE_KAISER:
      n = 2*self->GetInterpolationSizeParameter();
      break;
    }

  mode |= ((n - 1) << VTK_RESLICE_N_SHIFT);

  return mode;
}


//----------------------------------------------------------------------------
// rounding functions for each type, where 'F' is a floating-point type

#if (VTK_USE_INT8 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeInt8& rnd)
{
  rnd = vtkResliceRound(val);
}
#endif

#if (VTK_USE_UINT8 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeUInt8& rnd)
{
  rnd = vtkResliceRound(val);
}
#endif

#if (VTK_USE_INT16 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeInt16& rnd)
{
  rnd = vtkResliceRound(val);
}
#endif

#if (VTK_USE_UINT16 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeUInt16& rnd)
{
  rnd = vtkResliceRound(val);
}
#endif

#if (VTK_USE_INT32 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeInt32& rnd)
{
  rnd = vtkResliceRound(val);
}
#endif

#if (VTK_USE_UINT32 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeUInt32& rnd)
{
  rnd = vtkResliceRound(val);
}
#endif

#if (VTK_USE_FLOAT32 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeFloat32& rnd)
{
  rnd = val;
}
#endif

#if (VTK_USE_FLOAT64 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeFloat64& rnd)
{
  rnd = val;
}
#endif


//----------------------------------------------------------------------------
// clamping functions for each type

template <class F>
inline F vtkResliceClamp(F x, F xmin, F xmax)
{
  // do not change this code: it compiles into min/max opcodes
  x = (x > xmin ? x : xmin);
  x = (x < xmax ? x : xmax);
  return x;
}

#if (VTK_USE_INT8 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeInt8& clamp)
{
  static F minval = static_cast<F>(-128.0);
  static F maxval = static_cast<F>(127.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_UINT8 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeUInt8& clamp)
{
  static F minval = static_cast<F>(0);
  static F maxval = static_cast<F>(255.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_INT16 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeInt16& clamp)
{
  static F minval = static_cast<F>(-32768.0);
  static F maxval = static_cast<F>(32767.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_UINT16 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeUInt16& clamp)
{
  static F minval = static_cast<F>(0);
  static F maxval = static_cast<F>(65535.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_INT32 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeInt32& clamp)
{
  static F minval = static_cast<F>(-2147483648.0);
  static F maxval = static_cast<F>(2147483647.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_UINT32 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeUInt32& clamp)
{
  static F minval = static_cast<F>(0);
  static F maxval = static_cast<F>(4294967295.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_FLOAT32 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeFloat32& clamp)
{
  clamp = val;
}
#endif

#if (VTK_USE_FLOAT64 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeFloat64& clamp)
{
  clamp = val;
}
#endif


//----------------------------------------------------------------------------
// Convert from float to any type, with clamping or not.
template<class F, class T>
struct vtkImageResliceConversion
{
  static void Convert(
    void *&outPtrV, const F *inPtr, int numscalars, int n)
    {
    if (n > 0)
      {
      // This is a very hot loop, so it is unrolled
      T* outPtr = static_cast<T*>(outPtrV);
      int m = n*numscalars;
      for (int q = m >> 2; q > 0; --q)
        {
        vtkResliceRound(inPtr[0], outPtr[0]);
        vtkResliceRound(inPtr[1], outPtr[1]);
        vtkResliceRound(inPtr[2], outPtr[2]);
        vtkResliceRound(inPtr[3], outPtr[3]);
        inPtr += 4;
        outPtr += 4;
        }
      for (int r = m & 0x0003; r > 0; --r)
        {
        vtkResliceRound(*inPtr++, *outPtr++);
        }
      outPtrV = outPtr;
      }
    }

  static void Clamp(
    void *&outPtrV, const F *inPtr, int numscalars, int n)
    {
    T* outPtr = static_cast<T*>(outPtrV);
    for (int m = n*numscalars; m > 0; --m)
      {
      vtkResliceClamp(*inPtr++, *outPtr++);
      }
    outPtrV = outPtr;
    }
};


// get the convertion function
template<class F>
void vtkGetConversionFunc(vtkImageResliceBase *self,
                         void (**conversion)(void *&out, const F *in,
                                             int numscalars, int n))
{
  vtkImageData *input = static_cast<vtkImageData *>(self->GetInput());
  int inputType = input->GetScalarType();
  int dataType = self->GetOutput()->GetScalarType();

  if (self->GetInterpolationMode() <= VTK_RESLICE_LINEAR &&
      vtkDataArray::GetDataTypeMin(dataType) <=
        vtkDataArray::GetDataTypeMin(inputType) &&
      vtkDataArray::GetDataTypeMax(dataType) >=
        vtkDataArray::GetDataTypeMax(inputType))
    {
    // linear and nearest-neighbor do not need range checking
    switch (dataType)
      {
      vtkTemplateAliasMacro(
        *conversion = &(vtkImageResliceConversion<F, VTK_TT>::Convert)
        );
      default:
        *conversion = 0;
      }
    }
  else
    {
    // cubic interpolation needs range checking, so use clamp
    switch (dataType)
      {
      vtkTemplateAliasMacro(
        *conversion = &(vtkImageResliceConversion<F, VTK_TT>::Clamp)
        );
      default:
        *conversion = 0;
      }
    }
}


//----------------------------------------------------------------------------
// Perform a clamp to limit an index to [b, c] and subtract b.

inline int vtkInterpolateClamp(int a, int b, int c)
{
  a = (a <= c ? a : c);
  a -= b;
  a = (a >= 0 ? a : 0);
  return a;
}

//----------------------------------------------------------------------------
// Perform a wrap to limit an index to [b, c] and subtract b.

inline int vtkInterpolateWrap(int a, int b, int c)
{
  int range = c - b + 1;
  a -= b;
  a %= range;
  // required for some % implementations
  a = (a >= 0 ? a : a + range);
  return a;
}

//----------------------------------------------------------------------------
// Perform a wrap to limit an index to [0,range).
// Ensures correct behaviour when the index is negative.

inline int vtkInterpolateWrap(int num, int range)
{
  if ((num %= range) < 0)
    {
    num += range; // required for some % implementations
    }
  return num;
}

// Interpolate all three indices in-place
#define vtkInterpolateWrap3( idX, idY, idZ, rangeX, rangeY, rangeZ ) \
  idX = vtkInterpolateWrap( idX, rangeX ); \
  idY = vtkInterpolateWrap( idY, rangeY ); \
  idZ = vtkInterpolateWrap( idZ, rangeZ );


//----------------------------------------------------------------------------
// Perform a mirror to limit an index to [b, c] and subtract b.

inline int vtkInterpolateMirror(int a, int b, int c)
{
  int range1 = c - b;
  int range = range1 + 1;
  a -= b;
  a = (a >= 0 ? a : -a - 1);
  int count = a/range;
  a -= count*range;
  a = ((count & 0x1) == 0 ? a : range1 - a);
  return a;
}

//----------------------------------------------------------------------------
// Perform a mirror to limit an index to [0,range).

inline int vtkInterpolateMirror(int num, int range)
{
  if (num < 0)
    {
    num = -num - 1;
    }
  int count = num/range;
  num %= range;
  if (count & 0x1)
    {
    num = range - num - 1;
    }
  return num;
}

// Interpolate all three indices in-place
#define vtkInterpolateMirror3( idX, idY, idZ, rangeX, rangeY, rangeZ ) \
  idX = vtkInterpolateMirror( idX, rangeX ); \
  idY = vtkInterpolateMirror( idY, rangeY ); \
  idZ = vtkInterpolateMirror( idZ, rangeZ );


//----------------------------------------------------------------------------
// If the value is within one half voxel of the range [0,inExtX), then
// set it to "0" or "inExtX-1" as appropriate.
inline int vtkInterpolateBorder(int &inIdX0, int &inIdX1, int inExtX,
                                double fx)
{
  if (inIdX0 >= 0 && inIdX1 < inExtX)
    {
    return 0;
    }
  if (inIdX0 == -1 && fx >= 0.5)
    {
    inIdX1 = inIdX0 = 0;
    return 0;
    }
  if (inIdX0 == inExtX - 1 && fx < 0.5)
    {
    inIdX1 = inIdX0;
    return 0;
    }

  return 1;
}

inline int vtkInterpolateBorderCheck(int inIdX0, int inIdX1, int inExtX,
                                      double fx)
{
  if ((inIdX0 >= 0 && inIdX1 < inExtX) ||
      (inIdX0 == -1 && fx >= 0.5) ||
      (inIdX0 == inExtX - 1 && fx < 0.5))
    {
    return 0;
    }

  return 1;
}


//----------------------------------------------------------------------------
// Do tricubic interpolation of the input data 'inPtr' of extent 'inExt'
// at the 'point'.  The result is placed at 'outPtr'.
// If the lookup data is beyond the extent 'inExt', return 0,
// otherwise advance outPtr by numscalars.

// helper function: set up the lookup indices and the interpolation
// coefficients

template <class T>
void vtkTricubicInterpWeights(T F[4], int l, int h, T f)
{
  static const T half = T(0.5);

  if (l*h == 1)
    { // no interpolation
    F[0] = 0;
    F[1] = 1;
    F[2] = 0;
    F[3] = 0;
    return;
    }

  // cubic interpolation
  T fm1 = f - 1;
  T fd2 = f*half;
  T ft3 = f*3;
  F[0] = -fd2*fm1*fm1;
  F[1] = ((ft3 - 2)*fd2 - 1)*fm1;
  F[2] = -((ft3 - 4)*f - 1)*fd2;
  F[3] = f*fd2*fm1;

  if (h - l == 3)
    {
    return;
    }

  // if we are at an edge, extrapolate: edge pixel repeats

  if (l == 1)
    {
    F[1] += F[0];
    F[0] = 0;
    }
  if (l == 2)
    {
    F[2] += F[1];
    F[1] = 0;
    }

  if (h == 2)
    {
    F[2] += F[3];
    F[3] = 0;
    }
  if (h == 1)
    {
    F[1] += F[2];
    F[2] = 0;
    }
}


//--------------------------------------------------------------------------
// Check pointer memory alignment with 4-byte words
inline int vtkImageReslicePointerAlignment(void *ptr, int n)
{
#if (VTK_SIZEOF_VOID_P == 8)
  return ((reinterpret_cast<vtkTypeUInt64>(ptr) % n) == 0);
#else
  return ((reinterpret_cast<vtkTypeUInt32>(ptr) % n) == 0);
#endif
}


//--------------------------------------------------------------------------
// pixel copy function, templated for different scalar types
template <class T>
struct vtkImageResliceSetPixels
{
static void Set(void *&outPtrV, const void *inPtrV, int numscalars, int n)
{
  const T* inPtr = static_cast<const T*>(inPtrV);
  T* outPtr = static_cast<T*>(outPtrV);
  for (; n > 0; --n)
    {
    const T *tmpPtr = inPtr;
    int m = numscalars;
    do
      {
      *outPtr++ = *tmpPtr++;
      }
    while (--m);
    }
  outPtrV = outPtr;
}

// optimized for 1 scalar components
static void Set1(void *&outPtrV, const void *inPtrV,
                 int vtkNotUsed(numscalars), int n)
{
  const T* inPtr = static_cast<const T*>(inPtrV);
  T* outPtr = static_cast<T*>(outPtrV);
  T val = *inPtr;
  for (; n > 0; --n)
    {
    *outPtr++ = val;
    }
  outPtrV = outPtr;
}

// optimized for 2 scalar components
static void Set2(void *&outPtrV, const void *inPtrV,
                 int vtkNotUsed(numscalars), int n)
{
  const T* inPtr = static_cast<const T*>(inPtrV);
  T* outPtr = static_cast<T*>(outPtrV);
  for (; n > 0; --n)
    {
    outPtr[0] = inPtr[0];
    outPtr[1] = inPtr[1];
    outPtr += 2;
    }
  outPtrV = outPtr;
}

// optimized for 3 scalar components
static void Set3(void *&outPtrV, const void *inPtrV,
                 int vtkNotUsed(numscalars), int n)
{
  const T* inPtr = static_cast<const T*>(inPtrV);
  T* outPtr = static_cast<T*>(outPtrV);
  for (; n > 0; --n)
    {
    outPtr[0] = inPtr[0];
    outPtr[1] = inPtr[1];
    outPtr[2] = inPtr[2];
    outPtr += 3;
    }
  outPtrV = outPtr;
}

// optimized for 4 scalar components
static void Set4(void *&outPtrV, const void *inPtrV,
                 int vtkNotUsed(numscalars), int n)
{
  const T* inPtr = static_cast<const T*>(inPtrV);
  T* outPtr = static_cast<T*>(outPtrV);
  for (; n > 0; --n)
    {
    outPtr[0] = inPtr[0];
    outPtr[1] = inPtr[1];
    outPtr[2] = inPtr[2];
    outPtr[3] = inPtr[3];
    outPtr += 4;
    }
  outPtrV = outPtr;
}

};


//----------------------------------------------------------------------------
// get a pixel copy function that is appropriate for the data type
void vtkGetSetPixelsFunc(vtkImageResliceBase *self,
                         void (**setpixels)(void *&out, const void *in,
                                            int numscalars, int n));

//----------------------------------------------------------------------------
// Convert background color from float to appropriate type
template <class T>
void vtkCopyBackgroundColor(vtkImageResliceBase *self,
                            T *background, int numComponents)
{
  for (int i = 0; i < numComponents; i++)
    {
    if (i < 4)
      {
      vtkResliceClamp(self->GetBackgroundColor()[i], background[i]);
      }
    else
      {
      background[i] = 0;
      }
    }
}

//----------------------------------------------------------------------------
void vtkAllocBackgroundPixel(vtkImageResliceBase *self, void **rval,
                             int numComponents);

inline void vtkFreeBackgroundPixel(vtkImageResliceBase *vtkNotUsed(self), void **rval)
{
  double *doublePtr = static_cast<double *>(*rval);
  delete [] doublePtr;

  *rval = 0;
}

//----------------------------------------------------------------------------
// Methods to support windowed sinc interpolators

// sinc(x) from 0 to 8 with 256 bins per unit x
#define VTK_SINC_TABLE_SIZE ((VTK_RESLICE_MAX_KERNEL_SIZE + 2)*128 + 4)
static float vtkSincTable256[VTK_SINC_TABLE_SIZE];

inline void vtkBuildSincTable256()
{
  static int built = 0;

  if (built == 0)
    {
    vtkSincTable256[0] = 1.0;
    double p = vtkMath::DoublePi();
    double f = p/256.0;
    for (int i = 1; i < VTK_SINC_TABLE_SIZE; i++)
      {
      double x = i*f;
      vtkSincTable256[i] = sin(x)/x;
      }
    built = 1;
    }
}

template<class T>
T vtkSinc256(T x)
{
  // linear interpolation of sinc function
  T y = fabs(x);
  int i = static_cast<int>(y);
  T f = y - i;
  return (1 - f)*vtkSincTable256[i] + f*vtkSincTable256[i+1];
}

template<class T>
void vtkLanczosInterpWeights(T *F, T f, int m)
{
  // The table is only big enough for n=7
  if (m <= VTK_RESLICE_MAX_KERNEL_SIZE)
    {
    const T p = T(256); // table bins per unit
    int n = (m >> 1);
    T pn = p/n;
    T g = 1 - n - f;
    T x = p*g;
    T y = pn*g;
    T s = 0;
    int i = 0;
    do
      {
      T z = vtkSinc256(y)*vtkSinc256(x);
      s += z;
      F[i] = z;
      x += p;
      y += pn;
      }
    while (++i < m);

    // normalize
    s = 1/s;
    do
      {
      F[0] *= s;
      F[1] *= s;
      F += 2;
      }
    while (--n > 0);
    }
}

//----------------------------------------------------------------------------
// Compute the modified bessel function I0
static double vtkBesselI0(double x)
{
  int m = 0;
  double x2 = 0.25*x*x;
  double p = 1;
  double b = 1;
  do
    {
    m++;
    p *= x2/(m*m);
    b += p;
    }
  while (p > b*VTK_DBL_EPSILON);

  return b;
}

#define VTK_BESSEL_TABLE_SIZE ((VTK_RESLICE_MAX_KERNEL_SIZE + 2)*144 + 4)
static float vtkBesselTable96[VTK_BESSEL_TABLE_SIZE];

inline void vtkBuildBesselTable96()
{
  static int built = 0;

  if (built == 0)
    {
    for (int i = 0; i < VTK_BESSEL_TABLE_SIZE; i++)
      {
      vtkBesselTable96[i] = vtkBesselI0(i/96.0);
      }
    built = 1;
    }
}

template<class T>
T vtkBessel96(T x)
{
  // linear interpolation of bessel from the table
  int i = static_cast<int>(x);
  T f = x - i;
  return (1 - f)*vtkBesselTable96[i] + f*vtkBesselTable96[i+1];
}

template<class T>
void vtkKaiserInterpWeights(T *F, T f, int m)
{
  if (m <= VTK_RESLICE_MAX_KERNEL_SIZE)
    {
    // The Kaiser window has a tunable parameter "alpha", where
    // a smaller alpha increases sharpness (and ringing) while a
    // larger alpha can cause blurring.  I set the alpha to 3*n,
    // which closely approximates the optimal alpha values shown in
    // Helwig Hauser, Eduard Groller, Thomas Theussl,
    // "Mastering Windows: Improving Reconstruction,"
    // IEEE Symposium on Volume Visualization and Graphics (VV 2000),
    // pp. 101-108, 2000
    int n = (m >> 1);
    T a = 3*n;
    T q = 1.0/vtkBessel96(a*96);
    T g = 1.0/(n*n);
    T x = 1 - n - f;
    T s = 0;
    int i = 0;
    do
      {
      T y = (1 - x*x*g);
      y *= (y > 0);
      T z = q*vtkBessel96(a*sqrt(y)*96)*vtkSinc256(x*256);
      s += z;
      F[i] = z;
      x++;
      }
    while (++i < m);

    // normalize
    s = 1/s;
    do
      {
      F[0] *= s;
      F[1] *= s;
      F += 2;
      }
    while (--n > 0);
    }
}


//----------------------------------------------------------------------------
// This function simply clears the entire output to the background color,
// for cases where the transformation places the output extent completely
// outside of the input extent.
void vtkImageResliceClearExecute(vtkImageResliceBase *self,
                                 vtkImageData *, void *,
                                 vtkImageData *outData, void *outPtr,
                                 int outExt[6], int threadId);


//--------------------------------------------------------------------------
// pixel cast function, templated for different scalar types
template <class T>
struct vtkResliceCastPixels
{
  static void Cast(void *&outPtrV, const double *inPtrV, int numscalars)
  {
    T* outPtr = static_cast<T*>(outPtrV);
    const double *tmpPtr = inPtrV;
    int m = numscalars;
    do
      {
      vtkResliceRound( *tmpPtr, *outPtr );
      ++tmpPtr;
      ++outPtr;
      }
    while (--m);
    outPtrV = outPtr;
  }

  // optimized for 1 scalar components
  static void Cast1(void *&outPtrV, const double *inPtrV,
                            int vtkNotUsed(numscalars) )
  {
    T * out = static_cast<T*>(outPtrV);
    double* inPtr = const_cast< double * >(inPtrV);
    vtkResliceRound( *inPtr, *out );
    outPtrV = ++out;
  }
};

// get a pixel cast with round function that is appropriate for the data type
void vtkGetCastPixelsFunc(vtkImageResliceBase *self,
          void (**castpixels)(void *&out, const double *in, int numscalars ));


//----------------------------------------------------------------------------
// application of the transform has different forms for fixed-point
// vs. floating-point
template<class F>
inline
void vtkResliceApplyTransform(vtkAbstractTransform *newtrans,
                              F inPoint[3], F inOrigin[3],
                              const F inInvSpacing[3])
{
  if (newtrans)
    {
    newtrans->InternalTransformPoint(inPoint, inPoint);
    inPoint[0] -= inOrigin[0];
    inPoint[1] -= inOrigin[1];
    inPoint[2] -= inOrigin[2];
    inPoint[0] *= inInvSpacing[0];
    inPoint[1] *= inInvSpacing[1];
    inPoint[2] *= inInvSpacing[2];
    }
}

//----------------------------------------------------------------------------
// check a matrix to see whether it is the identity matrix
int vtkIsIdentityMatrix(vtkMatrix4x4 *matrix);

//----------------------------------------------------------------------------
// Inplace increment a point along a direction
inline void vtkImageResliceIncrement( double p[3], double inc[3] )
{
  p[0] += inc[0];
  p[1] += inc[1];
  p[2] += inc[2];
}


#endif
