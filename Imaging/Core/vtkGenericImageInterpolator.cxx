/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericImageInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGenericImageInterpolator.h"
#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDataArrayAccessor.h"
#include "vtkImageData.h"
#include "vtkImageInterpolatorInternals.h"
#include "vtkObjectFactory.h"
#include "vtkTypeTraits.h"

#include "vtkTemplateAliasMacro.h"
// turn off 64-bit ints when templating over all types, because
// they cannot be faithfully represented by doubles
#undef VTK_USE_INT64
#define VTK_USE_INT64 0
#undef VTK_USE_UINT64
#define VTK_USE_UINT64 0

vtkStandardNewMacro(vtkGenericImageInterpolator);

//------------------------------------------------------------------------------
vtkGenericImageInterpolator::vtkGenericImageInterpolator() = default;

//------------------------------------------------------------------------------
vtkGenericImageInterpolator::~vtkGenericImageInterpolator() = default;

//------------------------------------------------------------------------------
void vtkGenericImageInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Interpolation subroutines and associated code
//------------------------------------------------------------------------------

namespace
{

//------------------------------------------------------------------------------
template <class F, class T>
struct vtkImageNLCInterpolate
{
  static void Nearest(vtkInterpolationInfo* info, const F point[3], F* outPtr);

  static void Trilinear(vtkInterpolationInfo* info, const F point[3], F* outPtr);

  static void Tricubic(vtkInterpolationInfo* info, const F point[3], F* outPtr);
};

//------------------------------------------------------------------------------
template <class F, class T>
void vtkImageNLCInterpolate<F, T>::Nearest(vtkInterpolationInfo* info, const F point[3], F* outPtr)
{
  // const T* inPtr = static_cast<const T*>(info->Pointer);
  vtkDataArrayAccessor<T> array(static_cast<T*>(info->Array));
  vtkIdType inIdx = info->Index;
  int* inExt = info->Extent;
  vtkIdType* inInc = info->Increments;
  int numscalars = info->NumberOfComponents;

  int inIdX0 = vtkInterpolationMath::Round(point[0]);
  int inIdY0 = vtkInterpolationMath::Round(point[1]);
  int inIdZ0 = vtkInterpolationMath::Round(point[2]);

  switch (info->BorderMode)
  {
    case VTK_IMAGE_BORDER_REPEAT:
      inIdX0 = vtkInterpolationMath::Wrap(inIdX0, inExt[0], inExt[1]);
      inIdY0 = vtkInterpolationMath::Wrap(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = vtkInterpolationMath::Wrap(inIdZ0, inExt[4], inExt[5]);
      break;

    case VTK_IMAGE_BORDER_MIRROR:
      inIdX0 = vtkInterpolationMath::Mirror(inIdX0, inExt[0], inExt[1]);
      inIdY0 = vtkInterpolationMath::Mirror(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = vtkInterpolationMath::Mirror(inIdZ0, inExt[4], inExt[5]);
      break;

    default:
      inIdX0 = vtkInterpolationMath::Clamp(inIdX0, inExt[0], inExt[1]);
      inIdY0 = vtkInterpolationMath::Clamp(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = vtkInterpolationMath::Clamp(inIdZ0, inExt[4], inExt[5]);
      break;
  }

  inIdx += inIdX0 * inInc[0] + inIdY0 * inInc[1] + inIdZ0 * inInc[2];

  vtkIdType i = 0;
  do
  {
    *outPtr++ = array.Get(inIdx, i++);
  } while (--numscalars);
}

//------------------------------------------------------------------------------
template <class F, class T>
void vtkImageNLCInterpolate<F, T>::Trilinear(
  vtkInterpolationInfo* info, const F point[3], F* outPtr)
{
  vtkDataArrayAccessor<T> array(static_cast<T*>(info->Array));
  vtkIdType inIdx = info->Index;
  int* inExt = info->Extent;
  vtkIdType* inInc = info->Increments;
  int numscalars = info->NumberOfComponents;

  F fx, fy, fz;
  int inIdX0 = vtkInterpolationMath::Floor(point[0], fx);
  int inIdY0 = vtkInterpolationMath::Floor(point[1], fy);
  int inIdZ0 = vtkInterpolationMath::Floor(point[2], fz);

  int inIdX1 = inIdX0 + (fx != 0);
  int inIdY1 = inIdY0 + (fy != 0);
  int inIdZ1 = inIdZ0 + (fz != 0);

  switch (info->BorderMode)
  {
    case VTK_IMAGE_BORDER_REPEAT:
      inIdX0 = vtkInterpolationMath::Wrap(inIdX0, inExt[0], inExt[1]);
      inIdY0 = vtkInterpolationMath::Wrap(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = vtkInterpolationMath::Wrap(inIdZ0, inExt[4], inExt[5]);

      inIdX1 = vtkInterpolationMath::Wrap(inIdX1, inExt[0], inExt[1]);
      inIdY1 = vtkInterpolationMath::Wrap(inIdY1, inExt[2], inExt[3]);
      inIdZ1 = vtkInterpolationMath::Wrap(inIdZ1, inExt[4], inExt[5]);
      break;

    case VTK_IMAGE_BORDER_MIRROR:
      inIdX0 = vtkInterpolationMath::Mirror(inIdX0, inExt[0], inExt[1]);
      inIdY0 = vtkInterpolationMath::Mirror(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = vtkInterpolationMath::Mirror(inIdZ0, inExt[4], inExt[5]);

      inIdX1 = vtkInterpolationMath::Mirror(inIdX1, inExt[0], inExt[1]);
      inIdY1 = vtkInterpolationMath::Mirror(inIdY1, inExt[2], inExt[3]);
      inIdZ1 = vtkInterpolationMath::Mirror(inIdZ1, inExt[4], inExt[5]);
      break;

    default:
      inIdX0 = vtkInterpolationMath::Clamp(inIdX0, inExt[0], inExt[1]);
      inIdY0 = vtkInterpolationMath::Clamp(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = vtkInterpolationMath::Clamp(inIdZ0, inExt[4], inExt[5]);

      inIdX1 = vtkInterpolationMath::Clamp(inIdX1, inExt[0], inExt[1]);
      inIdY1 = vtkInterpolationMath::Clamp(inIdY1, inExt[2], inExt[3]);
      inIdZ1 = vtkInterpolationMath::Clamp(inIdZ1, inExt[4], inExt[5]);
      break;
  }

  vtkIdType factX0 = inIdX0 * inInc[0];
  vtkIdType factX1 = inIdX1 * inInc[0];
  vtkIdType factY0 = inIdY0 * inInc[1];
  vtkIdType factY1 = inIdY1 * inInc[1];
  vtkIdType factZ0 = inIdZ0 * inInc[2];
  vtkIdType factZ1 = inIdZ1 * inInc[2];

  vtkIdType i00 = factY0 + factZ0;
  vtkIdType i01 = factY0 + factZ1;
  vtkIdType i10 = factY1 + factZ0;
  vtkIdType i11 = factY1 + factZ1;

  F rx = 1 - fx;
  F ry = 1 - fy;
  F rz = 1 - fz;

  F ryrz = ry * rz;
  F fyrz = fy * rz;
  F ryfz = ry * fz;
  F fyfz = fy * fz;

  const vtkIdType inIdx0 = inIdx + factX0;
  const vtkIdType inIdx1 = inIdx + factX1;

  vtkIdType i = 0;
  do
  {
    *outPtr++ = (rx *
        (ryrz * array.Get(inIdx0 + i00, i) + ryfz * array.Get(inIdx0 + i01, i) +
          fyrz * array.Get(inIdx0 + i10, i) + fyfz * array.Get(inIdx0 + i11, i)) +
      fx *
        (ryrz * array.Get(inIdx1 + i00, i) + ryfz * array.Get(inIdx1 + i01, i) +
          fyrz * array.Get(inIdx1 + i10, i) + fyfz * array.Get(inIdx1 + i11, i)));
    i++;
  } while (--numscalars);
}

//------------------------------------------------------------------------------
// cubic helper function: set up the lookup indices and the interpolation
// coefficients

template <class T>
inline void vtkTricubicInterpWeights(T F[4], T f)
{
  static const T half = T(0.5);

  // cubic interpolation
  T fm1 = f - 1;
  T fd2 = f * half;
  T ft3 = f * 3;
  F[0] = -fd2 * fm1 * fm1;
  F[1] = ((ft3 - 2) * fd2 - 1) * fm1;
  F[2] = -((ft3 - 4) * f - 1) * fd2;
  F[3] = f * fd2 * fm1;
}

//------------------------------------------------------------------------------
// tricubic interpolation
template <class F, class T>
void vtkImageNLCInterpolate<F, T>::Tricubic(vtkInterpolationInfo* info, const F point[3], F* outPtr)
{
  vtkDataArrayAccessor<T> array(static_cast<T*>(info->Array));
  vtkIdType inIdx = info->Index;
  int* inExt = info->Extent;
  vtkIdType* inInc = info->Increments;
  int numscalars = info->NumberOfComponents;

  F fx, fy, fz;
  int inIdX0 = vtkInterpolationMath::Floor(point[0], fx);
  int inIdY0 = vtkInterpolationMath::Floor(point[1], fy);
  int inIdZ0 = vtkInterpolationMath::Floor(point[2], fz);

  // change arrays into locals
  vtkIdType inIncX = inInc[0];
  vtkIdType inIncY = inInc[1];
  vtkIdType inIncZ = inInc[2];

  int minX = inExt[0];
  int maxX = inExt[1];
  int minY = inExt[2];
  int maxY = inExt[3];
  int minZ = inExt[4];
  int maxZ = inExt[5];

  // the memory offsets
  vtkIdType factX[4], factY[4], factZ[4];

  switch (info->BorderMode)
  {
    case VTK_IMAGE_BORDER_REPEAT:
      factX[0] = vtkInterpolationMath::Wrap(inIdX0 - 1, minX, maxX) * inIncX;
      factX[1] = vtkInterpolationMath::Wrap(inIdX0, minX, maxX) * inIncX;
      factX[2] = vtkInterpolationMath::Wrap(inIdX0 + 1, minX, maxX) * inIncX;
      factX[3] = vtkInterpolationMath::Wrap(inIdX0 + 2, minX, maxX) * inIncX;

      factY[0] = vtkInterpolationMath::Wrap(inIdY0 - 1, minY, maxY) * inIncY;
      factY[1] = vtkInterpolationMath::Wrap(inIdY0, minY, maxY) * inIncY;
      factY[2] = vtkInterpolationMath::Wrap(inIdY0 + 1, minY, maxY) * inIncY;
      factY[3] = vtkInterpolationMath::Wrap(inIdY0 + 2, minY, maxY) * inIncY;

      factZ[0] = vtkInterpolationMath::Wrap(inIdZ0 - 1, minZ, maxZ) * inIncZ;
      factZ[1] = vtkInterpolationMath::Wrap(inIdZ0, minZ, maxZ) * inIncZ;
      factZ[2] = vtkInterpolationMath::Wrap(inIdZ0 + 1, minZ, maxZ) * inIncZ;
      factZ[3] = vtkInterpolationMath::Wrap(inIdZ0 + 2, minZ, maxZ) * inIncZ;
      break;

    case VTK_IMAGE_BORDER_MIRROR:
      factX[0] = vtkInterpolationMath::Mirror(inIdX0 - 1, minX, maxX) * inIncX;
      factX[1] = vtkInterpolationMath::Mirror(inIdX0, minX, maxX) * inIncX;
      factX[2] = vtkInterpolationMath::Mirror(inIdX0 + 1, minX, maxX) * inIncX;
      factX[3] = vtkInterpolationMath::Mirror(inIdX0 + 2, minX, maxX) * inIncX;

      factY[0] = vtkInterpolationMath::Mirror(inIdY0 - 1, minY, maxY) * inIncY;
      factY[1] = vtkInterpolationMath::Mirror(inIdY0, minY, maxY) * inIncY;
      factY[2] = vtkInterpolationMath::Mirror(inIdY0 + 1, minY, maxY) * inIncY;
      factY[3] = vtkInterpolationMath::Mirror(inIdY0 + 2, minY, maxY) * inIncY;

      factZ[0] = vtkInterpolationMath::Mirror(inIdZ0 - 1, minZ, maxZ) * inIncZ;
      factZ[1] = vtkInterpolationMath::Mirror(inIdZ0, minZ, maxZ) * inIncZ;
      factZ[2] = vtkInterpolationMath::Mirror(inIdZ0 + 1, minZ, maxZ) * inIncZ;
      factZ[3] = vtkInterpolationMath::Mirror(inIdZ0 + 2, minZ, maxZ) * inIncZ;
      break;

    default:
      factX[0] = vtkInterpolationMath::Clamp(inIdX0 - 1, minX, maxX) * inIncX;
      factX[1] = vtkInterpolationMath::Clamp(inIdX0, minX, maxX) * inIncX;
      factX[2] = vtkInterpolationMath::Clamp(inIdX0 + 1, minX, maxX) * inIncX;
      factX[3] = vtkInterpolationMath::Clamp(inIdX0 + 2, minX, maxX) * inIncX;

      factY[0] = vtkInterpolationMath::Clamp(inIdY0 - 1, minY, maxY) * inIncY;
      factY[1] = vtkInterpolationMath::Clamp(inIdY0, minY, maxY) * inIncY;
      factY[2] = vtkInterpolationMath::Clamp(inIdY0 + 1, minY, maxY) * inIncY;
      factY[3] = vtkInterpolationMath::Clamp(inIdY0 + 2, minY, maxY) * inIncY;

      factZ[0] = vtkInterpolationMath::Clamp(inIdZ0 - 1, minZ, maxZ) * inIncZ;
      factZ[1] = vtkInterpolationMath::Clamp(inIdZ0, minZ, maxZ) * inIncZ;
      factZ[2] = vtkInterpolationMath::Clamp(inIdZ0 + 1, minZ, maxZ) * inIncZ;
      factZ[3] = vtkInterpolationMath::Clamp(inIdZ0 + 2, minZ, maxZ) * inIncZ;
      break;
  }

  // get the interpolation coefficients
  F fX[4], fY[4], fZ[4];
  vtkTricubicInterpWeights(fX, fx);
  vtkTricubicInterpWeights(fY, fy);
  vtkTricubicInterpWeights(fZ, fz);

  // check if only one slice in a particular direction
  int multipleY = (minY != maxY);
  int multipleZ = (minZ != maxZ);

  // or if fractional offset is zero
  multipleY &= (fy != 0);
  multipleZ &= (fz != 0);

  // the limits to use when doing the interpolation
  int j1 = 1 - multipleY;
  int j2 = 1 + 2 * multipleY;

  int k1 = 1 - multipleZ;
  int k2 = 1 + 2 * multipleZ;

  // if only one coefficient will be used
  if (multipleY == 0)
  {
    fY[1] = 1;
  }
  if (multipleZ == 0)
  {
    fZ[1] = 1;
  }

  vtkIdType i = 0;
  do // loop over components
  {
    F val = 0;
    int k = k1;
    do // loop over z
    {
      F ifz = fZ[k];
      vtkIdType factz = factZ[k];
      int j = j1;
      do // loop over y
      {
        F ify = fY[j];
        F fzy = ifz * ify;
        vtkIdType factzy = factz + factY[j];

        const vtkIdType tmpIdx = inIdx + factzy;
        val += fzy *
          (fX[0] * array.Get(tmpIdx + factX[0], i) + fX[1] * array.Get(tmpIdx + factX[1], i) +
            fX[2] * array.Get(tmpIdx + factX[2], i) + fX[3] * array.Get(tmpIdx + factX[3], i));
      } while (++j <= j2);
    } while (++k <= k2);

    *outPtr++ = val;
    i++;
  } while (--numscalars);
}

template <typename F>
using vtkDefaultImageNLCInterpolate = vtkImageNLCInterpolate<F, vtkDataArray>;

template <class F>
struct GetNearestFuncWorker
{
  void (*interpolate)(vtkInterpolationInfo*, const F[3], F*);
  template <typename ArrayType>
  void operator()(ArrayType*)
  {
    interpolate = &vtkImageNLCInterpolate<F, ArrayType>::Nearest;
  }
};

template <class F>
struct GetTrilinearFuncWorker
{
  void (*interpolate)(vtkInterpolationInfo*, const F[3], F*);
  template <typename ArrayType>
  void operator()(ArrayType*)
  {
    interpolate = &vtkImageNLCInterpolate<F, ArrayType>::Trilinear;
  }
};

template <class F>
struct GetTricubicFuncWorker
{
  void (*interpolate)(vtkInterpolationInfo*, const F[3], F*);
  template <typename ArrayType>
  void operator()(ArrayType*)
  {
    interpolate = &vtkImageNLCInterpolate<F, ArrayType>::Tricubic;
  }
};

//------------------------------------------------------------------------------
// Get the interpolation function for the specified data types
template <class F>
void vtkGenericImageInterpolatorGetInterpolationFunc(
  void (**interpolate)(vtkInterpolationInfo*, const F[3], F*), vtkDataArray* array,
  int interpolationMode)
{
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
  switch (interpolationMode)
  {
    case VTK_NEAREST_INTERPOLATION:
    {
      GetNearestFuncWorker<F> worker;
      if (!Dispatcher::Execute(array, worker))
      {
        worker(array);
      }
      *interpolate = worker.interpolate;
      break;
    }
    case VTK_LINEAR_INTERPOLATION:
    {
      GetTrilinearFuncWorker<F> worker;
      if (!Dispatcher::Execute(array, worker))
      {
        worker(array);
      }
      *interpolate = worker.interpolate;
      break;
    }
    case VTK_CUBIC_INTERPOLATION:
    {
      GetTricubicFuncWorker<F> worker;
      if (!Dispatcher::Execute(array, worker))
      {
        worker(array);
      }
      *interpolate = worker.interpolate;
      break;
    }
  }
}

//------------------------------------------------------------------------------
// Interpolation for precomputed weights

template <class F, class T>
struct vtkImageNLCRowInterpolate
{
  static void Nearest(
    vtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n);

  static void Trilinear(
    vtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n);

  static void Tricubic(
    vtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n);
};

//------------------------------------------------------------------------------
// helper function for nearest neighbor interpolation
template <class F, class T>
void vtkImageNLCRowInterpolate<F, T>::Nearest(
  vtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n)
{
  const vtkIdType* iX = weights->Positions[0] + idX;
  const vtkIdType* iY = weights->Positions[1] + idY;
  const vtkIdType* iZ = weights->Positions[2] + idZ;
  vtkDataArrayAccessor<T> array(static_cast<T*>(weights->Array));
  vtkIdType inIdx = weights->Index + iY[0] + iZ[0];

  // get the number of components per pixel
  int numscalars = weights->NumberOfComponents;

  // This is a hot loop.
  for (int i = n; i > 0; --i)
  {
    const vtkIdType tmpIdx = inIdx + iX[0];
    iX++;
    int c = 0;
    int m = numscalars;
    do
    {
      *outPtr++ = array.Get(tmpIdx, c++);
    } while (--m);
  }
}

//------------------------------------------------------------------------------
// helper function for linear interpolation
template <class F, class T>
void vtkImageNLCRowInterpolate<F, T>::Trilinear(
  vtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n)
{
  int stepX = weights->KernelSize[0];
  int stepY = weights->KernelSize[1];
  int stepZ = weights->KernelSize[2];
  idX *= stepX;
  idY *= stepY;
  idZ *= stepZ;
  const F* fX = static_cast<F*>(weights->Weights[0]) + idX;
  const F* fY = static_cast<F*>(weights->Weights[1]) + idY;
  const F* fZ = static_cast<F*>(weights->Weights[2]) + idZ;
  const vtkIdType* iX = weights->Positions[0] + idX;
  const vtkIdType* iY = weights->Positions[1] + idY;
  const vtkIdType* iZ = weights->Positions[2] + idZ;
  vtkDataArrayAccessor<T> array(static_cast<T*>(weights->Array));
  vtkIdType inIdx = weights->Index;

  // get the number of components per pixel
  int numscalars = weights->NumberOfComponents;

  // create a 2x2 bilinear kernel in local variables
  vtkIdType i00 = iY[0] + iZ[0];
  vtkIdType i01 = i00;
  vtkIdType i10 = i00;
  vtkIdType i11 = i00;

  F ry = static_cast<F>(1);
  F fy = static_cast<F>(0);
  F rz = static_cast<F>(1);
  F fz = static_cast<F>(0);

  if (stepY == 2)
  {
    i10 = iY[1] + iZ[0];
    i11 = i10;
    ry = fY[0];
    fy = fY[1];
  }

  if (stepZ == 2)
  {
    i01 = iY[0] + iZ[1];
    i11 = i01;
    rz = fZ[0];
    fz = fZ[1];
  }

  if (stepY + stepZ == 4)
  {
    i11 = iY[1] + iZ[1];
  }

  F ryrz = ry * rz;
  F ryfz = ry * fz;
  F fyrz = fy * rz;
  F fyfz = fy * fz;

  if (stepX == 1)
  {
    if (fy == 0 && fz == 0)
    { // no interpolation needed at all
      const vtkIdType inIdx1 = inIdx + i00;
      for (int i = n; i > 0; --i)
      {
        const vtkIdType inIdx0 = inIdx1 + *iX++;
        int c = 0;
        int m = numscalars;
        do
        {
          *outPtr++ = array.Get(inIdx0, c++);
        } while (--m);
      }
    }
    else if (fy == 0)
    { // only need linear z interpolation
      for (int i = n; i > 0; --i)
      {
        const vtkIdType inIdx0 = inIdx + *iX++;
        int c = 0;
        int m = numscalars;
        do
        {
          *outPtr++ = (rz * array.Get(inIdx0 + i00, c) + fz * array.Get(inIdx0 + i01, c));
          c++;
        } while (--m);
      }
    }
    else
    { // interpolate in y and z but not in x
      for (int i = n; i > 0; --i)
      {
        const vtkIdType inIdx0 = inIdx + *iX++;
        int c = 0;
        int m = numscalars;
        do
        {
          *outPtr++ = (ryrz * array.Get(inIdx0 + i00, c) + ryfz * array.Get(inIdx0 + i01, c) +
            fyrz * array.Get(inIdx0 + i10, c) + fyfz * array.Get(inIdx0 + i11, c));
          c++;
        } while (--m);
      }
    }
  }
  else if (fz == 0)
  { // bilinear interpolation in x,y
    for (int i = n; i > 0; --i)
    {
      F rx = fX[0];
      F fx = fX[1];
      fX += 2;

      vtkIdType t0 = iX[0];
      vtkIdType t1 = iX[1];
      iX += 2;

      const vtkIdType inIdx0 = inIdx + t0;
      const vtkIdType inIdx1 = inIdx + t1;
      int c = 0;
      int m = numscalars;
      do
      {
        *outPtr++ = (rx * (ry * array.Get(inIdx0 + i00, c) + fy * array.Get(inIdx0 + i10, c)) +
          fx * (ry * array.Get(inIdx1 + i00, c) + fy * array.Get(inIdx1 + i10, c)));
        c++;
      } while (--m);
    }
  }
  else
  { // do full trilinear interpolation
    for (int i = n; i > 0; --i)
    {
      F rx = fX[0];
      F fx = fX[1];
      fX += 2;

      vtkIdType t0 = iX[0];
      vtkIdType t1 = iX[1];
      iX += 2;

      const vtkIdType inIdx0 = inIdx + t0;
      const vtkIdType inIdx1 = inIdx + t1;
      int c = 0;
      int m = numscalars;
      do
      {
        *outPtr++ = (rx *
            (ryrz * array.Get(inIdx0 + i00, c) + ryfz * array.Get(inIdx0 + i01, c) +
              fyrz * array.Get(inIdx0 + i10, c) + fyfz * array.Get(inIdx0 + i11, c)) +
          fx *
            (ryrz * array.Get(inIdx1 + i00, c) + ryfz * array.Get(inIdx1 + i01, c) +
              fyrz * array.Get(inIdx1 + i10, c) + fyfz * array.Get(inIdx1 + i11, c)));
        c++;
      } while (--m);
    }
  }
}

//------------------------------------------------------------------------------
// helper function for tricubic interpolation
template <class F, class T>
void vtkImageNLCRowInterpolate<F, T>::Tricubic(
  vtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n)
{
  int stepX = weights->KernelSize[0];
  int stepY = weights->KernelSize[1];
  int stepZ = weights->KernelSize[2];
  idX *= stepX;
  idY *= stepY;
  idZ *= stepZ;
  const F* fX = static_cast<F*>(weights->Weights[0]) + idX;
  const F* fY = static_cast<F*>(weights->Weights[1]) + idY;
  const F* fZ = static_cast<F*>(weights->Weights[2]) + idZ;
  const vtkIdType* iX = weights->Positions[0] + idX;
  const vtkIdType* iY = weights->Positions[1] + idY;
  const vtkIdType* iZ = weights->Positions[2] + idZ;
  vtkDataArrayAccessor<T> array(static_cast<T*>(weights->Array));
  vtkIdType inIdx = weights->Index;

  // get the number of components per pixel
  int numscalars = weights->NumberOfComponents;

  for (int i = n; i > 0; --i)
  {
    vtkIdType iX0 = iX[0];
    vtkIdType iX1 = iX0;
    vtkIdType iX2 = iX0;
    vtkIdType iX3 = iX0;
    F fX0 = static_cast<F>(1);
    F fX1 = static_cast<F>(0);
    F fX2 = fX1;
    F fX3 = fX1;

    switch (stepX)
    {
      case 4:
        iX3 = iX[3];
        fX3 = fX[3];
        VTK_FALLTHROUGH;
      case 3:
        iX2 = iX[2];
        fX2 = fX[2];
        VTK_FALLTHROUGH;
      case 2:
        iX1 = iX[1];
        fX1 = fX[1];
        fX0 = fX[0];
    }

    iX += stepX;
    fX += stepX;

    const vtkIdType inIdx0 = inIdx;
    int cc = 0;
    int c = numscalars;
    do
    { // loop over components
      F result = 0;

      int k = 0;
      do
      { // loop over z
        F fz = fZ[k];
        if (fz != 0)
        {
          vtkIdType iz = iZ[k];
          int j = 0;
          do
          { // loop over y
            F fy = fY[j];
            F fzy = fz * fy;
            vtkIdType izy = iz + iY[j];
            const vtkIdType tmpIdx = inIdx0 + izy;
            // loop over x is unrolled (significant performance boost)
            result += fzy *
              (fX0 * array.Get(tmpIdx + iX0, cc) + fX1 * array.Get(tmpIdx + iX1, cc) +
                fX2 * array.Get(tmpIdx + iX2, cc) + fX3 * array.Get(tmpIdx + iX3, cc));
          } while (++j < stepY);
        }
      } while (++k < stepZ);

      *outPtr++ = result;
      cc++;
      ;
    } while (--c);
  }
}

template <typename F>
using vtkDefaultImageNLCRowInterpolate = vtkImageNLCRowInterpolate<F, vtkDataArray>;

template <class F>
struct GetNearestRowFuncWorker
{
  void (*summation)(vtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n);
  template <typename ArrayType>
  void operator()(ArrayType*)
  {
    summation = &vtkImageNLCRowInterpolate<F, ArrayType>::Nearest;
  }
};

template <class F>
struct GetTrilinearRowFuncWorker
{
  void (*summation)(vtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n);
  template <typename ArrayType>
  void operator()(ArrayType*)
  {
    summation = &vtkImageNLCRowInterpolate<F, ArrayType>::Trilinear;
  }
};

template <class F>
struct GetTricubicRowFuncWorker
{
  void (*summation)(vtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n);
  template <typename ArrayType>
  void operator()(ArrayType*)
  {
    summation = &vtkImageNLCRowInterpolate<F, ArrayType>::Tricubic;
  }
};

//------------------------------------------------------------------------------
// get row interpolation function for different interpolation modes
// and different scalar types
template <class F>
void vtkGenericImageInterpolatorGetRowInterpolationFunc(
  void (**summation)(vtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n),
  vtkDataArray* array, int interpolationMode)
{
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
  switch (interpolationMode)
  {
    case VTK_NEAREST_INTERPOLATION:
    {
      GetNearestRowFuncWorker<F> worker;
      if (!Dispatcher::Execute(array, worker))
      {
        worker(array);
      }
      *summation = worker.summation;
      break;
    }
    case VTK_LINEAR_INTERPOLATION:
    {
      GetTrilinearRowFuncWorker<F> worker;
      if (!Dispatcher::Execute(array, worker))
      {
        worker(array);
      }
      *summation = worker.summation;
      break;
    }
    case VTK_CUBIC_INTERPOLATION:
    {
      GetTricubicRowFuncWorker<F> worker;
      if (!Dispatcher::Execute(array, worker))
      {
        worker(array);
      }
      *summation = worker.summation;
      break;
    }
  }
}

//------------------------------------------------------------------------------
} // ends anonymous namespace

//------------------------------------------------------------------------------
void vtkGenericImageInterpolator::GetInterpolationFunc(
  void (**func)(vtkInterpolationInfo*, const double[3], double*))
{
  vtkGenericImageInterpolatorGetInterpolationFunc(
    func, this->InterpolationInfo->Array, this->InterpolationMode);
}

//------------------------------------------------------------------------------
void vtkGenericImageInterpolator::GetInterpolationFunc(
  void (**func)(vtkInterpolationInfo*, const float[3], float*))
{
  vtkGenericImageInterpolatorGetInterpolationFunc(
    func, this->InterpolationInfo->Array, this->InterpolationMode);
}

//------------------------------------------------------------------------------
void vtkGenericImageInterpolator::GetRowInterpolationFunc(
  void (**func)(vtkInterpolationWeights*, int, int, int, double*, int))
{
  vtkGenericImageInterpolatorGetRowInterpolationFunc(
    func, this->InterpolationInfo->Array, this->InterpolationMode);
}

//------------------------------------------------------------------------------
void vtkGenericImageInterpolator::GetRowInterpolationFunc(
  void (**func)(vtkInterpolationWeights*, int, int, int, float*, int))
{
  vtkGenericImageInterpolatorGetRowInterpolationFunc(
    func, this->InterpolationInfo->Array, this->InterpolationMode);
}

//------------------------------------------------------------------------------
// default do-nothing interpolation functions
namespace
{

template <class F>
struct vtkInterpolateNOP
{
  static void InterpolationFunc(vtkInterpolationInfo* info, const F point[3], F* outPtr);

  static void RowInterpolationFunc(
    vtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n);
};

template <class F>
void vtkInterpolateNOP<F>::InterpolationFunc(vtkInterpolationInfo*, const F[3], F*)
{
}

template <class F>
void vtkInterpolateNOP<F>::RowInterpolationFunc(vtkInterpolationWeights*, int, int, int, F*, int)
{
}

} // end anonymous namespace

//------------------------------------------------------------------------------

// This is a copy/paste from vtkAbstractInterpolator::Update() with the
// assignment of the data array's pointer removed.
void vtkGenericImageInterpolator::Update()
{
  vtkDataArray* scalars = this->Scalars;

  // check for scalars
  if (!scalars)
  {
    this->InterpolationInfo->Pointer = nullptr;
    this->InterpolationInfo->NumberOfComponents = 1;

    this->InterpolationInfo->Array = nullptr;
    this->InterpolationInfo->Index = 0;

    this->InterpolationFuncDouble = &(vtkInterpolateNOP<double>::InterpolationFunc);
    this->InterpolationFuncFloat = &(vtkInterpolateNOP<float>::InterpolationFunc);
    this->RowInterpolationFuncDouble = &(vtkInterpolateNOP<double>::RowInterpolationFunc);
    this->RowInterpolationFuncFloat = &(vtkInterpolateNOP<float>::RowInterpolationFunc);

    return;
  }

  // set the InterpolationInfo object
  vtkInterpolationInfo* info = this->InterpolationInfo;
  vtkIdType* inc = info->Increments;
  int* extent = info->Extent;
  extent[0] = this->Extent[0];
  extent[1] = this->Extent[1];
  extent[2] = this->Extent[2];
  extent[3] = this->Extent[3];
  extent[4] = this->Extent[4];
  extent[5] = this->Extent[5];

  // use the Extent and Tolerance to set the bounds
  double* bounds = this->StructuredBoundsDouble;
  float* fbounds = this->StructuredBoundsFloat;
  double tol = this->Tolerance;
  // always restrict the bounds to the limits of int
  int supportSize[3];
  this->ComputeSupportSize(nullptr, supportSize);
  // use the max of the three support size values
  int kernelSize = supportSize[0];
  kernelSize = ((supportSize[1] < kernelSize) ? kernelSize : supportSize[1]);
  kernelSize = ((supportSize[2] < kernelSize) ? kernelSize : supportSize[2]);
  double minbound = VTK_INT_MIN + kernelSize / 2;
  double maxbound = VTK_INT_MAX - kernelSize / 2;

  for (int i = 0; i < 3; i++)
  {
    // use min tolerance of 0.5 if just one slice thick
    double newtol = 0.5 * (extent[2 * i] == extent[2 * i + 1]);
    newtol = ((newtol > tol) ? newtol : tol);

    double bound = extent[2 * i] - newtol;
    bound = ((bound > minbound) ? bound : minbound);
    fbounds[2 * i] = bounds[2 * i] = bound;
    bound = extent[2 * i + 1] + newtol;
    bound = ((bound < maxbound) ? bound : maxbound);
    fbounds[2 * i + 1] = bounds[2 * i + 1] = bound;
  }

  // generate the increments
  int xdim = extent[1] - extent[0] + 1;
  int ydim = extent[3] - extent[2] + 1;

  int ncomp = scalars->GetNumberOfComponents();
  inc[0] = ncomp;
  inc[1] = inc[0] * xdim;
  inc[2] = inc[1] * ydim;

  // compute first component and adjust data pointer
  int component = this->ComponentOffset;
  component = ((component > 0) ? component : 0);
  component = ((component < ncomp) ? component : ncomp - 1);

  int dataSize = scalars->GetDataTypeSize();

  info->Array = scalars;
  info->Index = component * dataSize;

  // set all other elements of the InterpolationInfo
  info->ScalarType = scalars->GetDataType();
  info->NumberOfComponents = this->ComputeNumberOfComponents(ncomp);
  info->BorderMode = this->BorderMode;

  // subclass-specific update
  this->InternalUpdate();

  // get the functions that will perform the interpolation
  this->GetInterpolationFunc(&this->InterpolationFuncDouble);
  this->GetInterpolationFunc(&this->InterpolationFuncFloat);

  // we must assign both of these function types, since the default
  // implementation of each relies on GetVoidPointer()
  this->GetSlidingWindowFunc(&this->RowInterpolationFuncDouble);
  this->GetSlidingWindowFunc(&this->RowInterpolationFuncFloat);
  this->GetRowInterpolationFunc(&this->RowInterpolationFuncDouble);
  this->GetRowInterpolationFunc(&this->RowInterpolationFuncFloat);
}
