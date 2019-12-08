/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractImageInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractImageInterpolator.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkImageInterpolatorInternals.h"
#include "vtkPointData.h"
#include "vtkTypeTraits.h"

#include "vtkTemplateAliasMacro.h"
// turn off 64-bit ints when templating over all types, because
// they cannot be faithfully represented by doubles
#undef VTK_USE_INT64
#define VTK_USE_INT64 0
#undef VTK_USE_UINT64
#define VTK_USE_UINT64 0

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
vtkAbstractImageInterpolator::vtkAbstractImageInterpolator()
{
  this->Scalars = nullptr;
  this->BorderMode = VTK_IMAGE_BORDER_CLAMP;
  this->SlidingWindow = false;

  for (int i = 0; i < 6; i++)
  {
    this->StructuredBoundsDouble[i] = 0.0;
    this->StructuredBoundsFloat[i] = 0.0f;
  }

  for (int j = 0; j < 3; j++)
  {
    this->Extent[2 * j] = 0;
    this->Extent[2 * j + 1] = -1;
    this->Spacing[j] = 1.0;
    this->Origin[j] = 0.0;
  }

  this->OutValue = 0.0;
  this->Tolerance = 7.62939453125e-06;
  this->ComponentOffset = 0;
  this->ComponentCount = -1;

  this->InterpolationInfo = new vtkInterpolationInfo();
  this->InterpolationInfo->Pointer = nullptr;
  this->InterpolationInfo->NumberOfComponents = 1;
  this->InterpolationInfo->InterpolationMode = 0;
  this->InterpolationInfo->ExtraInfo = nullptr;

  this->InterpolationFuncDouble = &(vtkInterpolateNOP<double>::InterpolationFunc);
  this->InterpolationFuncFloat = &(vtkInterpolateNOP<float>::InterpolationFunc);
  this->RowInterpolationFuncDouble = &(vtkInterpolateNOP<double>::RowInterpolationFunc);
  this->RowInterpolationFuncFloat = &(vtkInterpolateNOP<float>::RowInterpolationFunc);
}

//----------------------------------------------------------------------------
vtkAbstractImageInterpolator::~vtkAbstractImageInterpolator()
{
  if (this->Scalars)
  {
    this->Scalars->Delete();
  }
  delete this->InterpolationInfo;
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::DeepCopy(vtkAbstractImageInterpolator* obj)
{
  this->SetTolerance(obj->Tolerance);
  this->SetOutValue(obj->OutValue);
  this->SetComponentOffset(obj->ComponentOffset);
  this->SetComponentCount(obj->ComponentCount);
  this->SetBorderMode(obj->BorderMode);
  this->SetSlidingWindow(obj->SlidingWindow);
  obj->GetExtent(this->Extent);
  obj->GetOrigin(this->Origin);
  obj->GetSpacing(this->Spacing);
  if (this->Scalars)
  {
    this->Scalars->Delete();
    this->Scalars = nullptr;
  }
  if (obj->Scalars)
  {
    this->Scalars = obj->Scalars;
    this->Scalars->Register(this);
  }
  *this->InterpolationInfo = *obj->InterpolationInfo;
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "OutValue: " << this->OutValue << "\n";
  os << indent << "ComponentOffset: " << this->ComponentOffset << "\n";
  os << indent << "ComponentCount: " << this->ComponentCount << "\n";
  os << indent << "BorderMode: " << this->GetBorderModeAsString() << "\n";
  os << indent << "SlidingWindow: " << (this->SlidingWindow ? "On\n" : "Off\n");
  os << indent << "Extent: " << this->Extent[0] << " " << this->Extent[1] << " " << this->Extent[2]
     << " " << this->Extent[3] << " " << this->Extent[4] << " " << this->Extent[5] << "\n";
  os << indent << "Origin: " << this->Origin[0] << " " << this->Origin[1] << " " << this->Origin[2]
     << "\n";
  os << indent << "Spacing: " << this->Spacing[0] << " " << this->Spacing[1] << " "
     << this->Spacing[2] << "\n";
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::SetBorderMode(int mode)
{
  mode = vtkMath::ClampValue(mode, VTK_IMAGE_BORDER_CLAMP, VTK_IMAGE_BORDER_MIRROR);
  if (this->BorderMode != mode)
  {
    this->BorderMode = mode;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char* vtkAbstractImageInterpolator::GetBorderModeAsString()
{
  switch (this->BorderMode)
  {
    case VTK_IMAGE_BORDER_CLAMP:
      return "Clamp";
    case VTK_IMAGE_BORDER_REPEAT:
      return "Repeat";
    case VTK_IMAGE_BORDER_MIRROR:
      return "Mirror";
  }
  return "";
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::SetComponentOffset(int offset)
{
  if (this->ComponentOffset != offset)
  {
    this->ComponentOffset = offset;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::SetComponentCount(int count)
{
  if (this->ComponentCount != count)
  {
    this->ComponentCount = count;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkAbstractImageInterpolator::ComputeNumberOfComponents(int inputCount)
{
  // validate the component range to extract
  int component = this->ComponentOffset;
  int count = this->ComponentCount;

  component = ((component > 0) ? component : 0);
  component = ((component < inputCount) ? component : inputCount - 1);
  count = ((count < (inputCount - component)) ? count : (inputCount - component));
  count = ((count > 0) ? count : (inputCount - component));

  return count;
}

//----------------------------------------------------------------------------
int vtkAbstractImageInterpolator::GetNumberOfComponents()
{
  return this->InterpolationInfo->NumberOfComponents;
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::SetOutValue(double value)
{
  if (this->OutValue != value)
  {
    this->OutValue = value;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::SetTolerance(double value)
{
  if (this->Tolerance != value)
  {
    this->Tolerance = value;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::SetSlidingWindow(bool x)
{
  if (this->SlidingWindow != x)
  {
    this->SlidingWindow = x;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::Initialize(vtkDataObject* o)
{
  // free any previous scalars
  this->ReleaseData();

  // check for valid data
  vtkImageData* data = vtkImageData::SafeDownCast(o);
  vtkDataArray* scalars = nullptr;
  if (data)
  {
    scalars = data->GetPointData()->GetScalars();
  }

  if (data == nullptr || scalars == nullptr)
  {
    vtkErrorMacro("Initialize(): no image data to interpolate!");
    return;
  }

  // claim the scalars
  scalars->Register(this);
  this->Scalars = scalars;

  // get the image information
  data->GetSpacing(this->Spacing);
  data->GetOrigin(this->Origin);
  data->GetExtent(this->Extent);

  // call update
  this->Update();
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::ReleaseData()
{
  if (this->Scalars)
  {
    this->Scalars->Delete();
    this->Scalars = nullptr;
  }
}

namespace
{

//----------------------------------------------------------------------------
template <class F>
void vtkSlidingWindowAllocateWorkspace(vtkInterpolationWeights* weights, F*)
{
  int* extent = weights->WeightExtent;

  int kernelSizeX = weights->KernelSize[0];
  int kernelSizeY = weights->KernelSize[1];
  int kernelSizeZ = weights->KernelSize[2];

  vtkIdType rowSize = (extent[1] - extent[0] + 1);
  rowSize *= weights->NumberOfComponents;

  // workPtr2 is a cache for partial sums
  F* workPtr2;
  // workPtr provides an index into the cache
  F** workPtr;

  if (kernelSizeX == 1 && kernelSizeY == 1 && kernelSizeZ == 1)
  {
    // no cache is needed for a 1x1x1 kernel
    workPtr2 = nullptr;
    workPtr = nullptr;
  }
  else if (kernelSizeZ == 1)
  {
    // allocate workspace for a 2D sliding window
    vtkIdType workSize = rowSize * kernelSizeY;
    workPtr2 = new F[workSize];

    // allocate the sliding row pointers
    workPtr = new F*[kernelSizeY];
    for (int i = 0; i < kernelSizeY; i++)
    {
      workPtr[i] = workPtr2 + i * rowSize;
    }
  }
  else
  {
    // allocate workspace for sliding window (slices and rows)
    vtkIdType sliceSize = rowSize * (extent[3] - extent[2] + 1);
    vtkIdType workSize = rowSize * kernelSizeY;
    workSize += sliceSize * kernelSizeZ;
    workPtr2 = new F[workSize];

    // allocate the sliding row and slice pointers
    workPtr = new F*[kernelSizeZ + kernelSizeY];
    // sliding rows
    for (int i = 0; i < kernelSizeY; i++)
    {
      workPtr[i] = workPtr2 + i * rowSize;
    }
    // sliding slices
    F** slicePtr = workPtr + kernelSizeY;
    F* workPtr3 = workPtr2 + kernelSizeY * rowSize;
    for (int i = 0; i < kernelSizeZ; i++)
    {
      slicePtr[i] = workPtr3 + i * sliceSize;
    }
  }

  // make the workspace available to the interpolator
  weights->Workspace = workPtr;

  // initialize the "last position" of the sliding window to invalid
  // values (i.e. to a position outside of the extent)
  weights->LastY = extent[2] - 1;
  weights->LastZ = extent[4] - 1;
}

//----------------------------------------------------------------------------
// Apply a 1D filter in the X direction.
// The inPtr parameter must be positioned at the correct slice.
template <class T, class F>
void vtkSlidingWindowX(const T* inPtr, F* outPtr, int pixelCount, int ncomp, const vtkIdType* a,
  const F* f, int kernelSize)
{
  if (kernelSize == 1)
  {
    for (int i = 0; i < pixelCount; i++)
    {
      // 'a' maps output pixel to input pixel in the x direction
      const T* tmpPtr = inPtr + (*a++);
      for (int j = 0; j < ncomp; j++)
      {
        *outPtr++ = *tmpPtr++;
      }
    }
  }
  else
  {
    for (int i = 0; i < pixelCount; i++)
    {
      const T* tmpPtr = inPtr;
      for (int j = 0; j < ncomp; j++)
      {
        // do the convolution sum in the x direction, where
        // 'a' gives the input pixel positions and
        // 'g' gives the weights
        int k = kernelSize - 1;
        const vtkIdType* b = a;
        const F* g = f;
        F val = (*g++) * tmpPtr[*b++];
        do
        {
          val += (*g++) * tmpPtr[*b++];
        } while (--k);
        tmpPtr++;
        *outPtr++ = val;
      }
      a += kernelSize;
      f += kernelSize;
    }
  }
}

//----------------------------------------------------------------------------
// Apply a 1D filter along the Y or Z direction, given kernelSize rows
// of data as input and producing one row of data as output.  This function
// must be called for each row of the output to filter a whole slice.
template <class F>
void vtkSlidingWindowYOrZ(
  F** rowPtr, F* outPtr, vtkIdType begin, vtkIdType end, const F* f, int kernelSize)
{
  if (kernelSize == 1)
  {
    // don't apply the filter, just copy the data
    F* tmpPtr = *rowPtr + begin;
    for (vtkIdType i = begin; i < end; i++)
    {
      *outPtr++ = *tmpPtr++;
    }
  }
  else
  {
    // apply the filter to one row of the image
    for (vtkIdType i = begin; i < end; i++)
    {
      int k = kernelSize - 1;
      F** tmpPtr = rowPtr;
      // 'f' is an array of weights
      const F* g = f;
      F val = (*g++) * ((*tmpPtr++)[i]);
      do
      {
        val += (*g++) * ((*tmpPtr++)[i]);
      } while (--k);
      *outPtr++ = val;
    }
  }
}

//----------------------------------------------------------------------------
// Apply a 2D filter to image slices,
// The inPtr parameter must be positioned at the correct slice.
template <class T, class F>
void vtkSlidingWindow2D(const T* inPtr, F* outPtr, const int extent[6], int idX, int idY,
  int lastIdY, int pixelCount, int ncomp, const vtkIdType* aX, const F* fX, int kernelSizeX,
  const vtkIdType* aY, const F* fY, int kernelSizeY, F** workPtr)
{
  int extentX = extent[1] - extent[0] + 1;
  vtkIdType outIncX = ncomp;
  vtkIdType begin = (idX - extent[0]) * outIncX;
  vtkIdType end = begin + pixelCount * outIncX;

  if (kernelSizeY == 1)
  {
    // filter only in the X direction
    vtkSlidingWindowX(&inPtr[*aY], *workPtr, extentX, ncomp, aX, fX, kernelSizeX);

    // this becomes a copy operation since kernelSizeY is 1
    vtkSlidingWindowYOrZ(workPtr, outPtr, begin, end, fY, kernelSizeY);
  }
  else
  {
    // filter in both X and Y directions,
    // 'n' is the number of rows of partial sums that can be reused from
    // the previous y position
    int n = 0;
    // if lastIdY was outside of extent, then this is the first iteration
    if (lastIdY >= extent[2])
    {
      const vtkIdType* lY = aY - (idY - lastIdY) * kernelSizeY;
      for (int j = 0; j < kernelSizeY; j++)
      {
        const vtkIdType* bY = lY + j;
        const vtkIdType* cY = aY;
        int i = kernelSizeY - j;
        do
        {
          if (*cY++ != *bY++)
          {
            break;
          }
        } while (--i);
        if (i == 0)
        {
          // if all indices after offset 'j' match, then 'j' rows worth of
          // partial sums have to be recomputed and 'n' rows are reusable
          n = kernelSizeY - j;
          break;
        }
      }
    }

    // rotate workspace rows to reuse the rows that can be reused
    if (n < kernelSizeY)
    {
      for (int k = 0; k < n; k++)
      {
        int j = kernelSizeY + k - n;
        F* tmpPtr = workPtr[k];
        workPtr[k] = workPtr[j];
        workPtr[j] = tmpPtr;
      }

      // compute the new rows that are needed
      for (int k = n; k < kernelSizeY; k++)
      {
        vtkSlidingWindowX(&inPtr[aY[k]], workPtr[k], extentX, ncomp, aX, fX, kernelSizeX);
      }
    }

    vtkSlidingWindowYOrZ(workPtr, outPtr, begin, end, fY, kernelSizeY);
  }
}

//----------------------------------------------------------------------------
template <class F, class T>
struct vtkSlidingWindow
{
  static void InterpolateRow(
    vtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n);
};

//----------------------------------------------------------------------------
// Apply separable blur filter fX, fY, fZ to an image with minimum
// memory overhead (3 rows of temp storage for 2D, 3 slices for 3D).
// The aX, aY, and aZ contain increments for the X, Y, and Z
// directions.
template <class F, class T>
void vtkSlidingWindow<F, T>::InterpolateRow(
  vtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int pixelCount)
{
  if (!weights->Workspace)
  {
    vtkSlidingWindowAllocateWorkspace(weights, outPtr);
  }

  int lastIdY = weights->LastY;
  int lastIdZ = weights->LastZ;
  weights->LastY = idY;
  weights->LastZ = idZ;

  const T* inPtr = static_cast<const T*>(weights->Pointer);
  F** workPtr = static_cast<F**>(weights->Workspace);

  const int* extent = weights->WeightExtent;
  int ncomp = weights->NumberOfComponents;

  int kernelSizeX = weights->KernelSize[0];
  int kernelSizeY = weights->KernelSize[1];
  int kernelSizeZ = weights->KernelSize[2];

  const vtkIdType* aX = weights->Positions[0];
  const vtkIdType* aY = weights->Positions[1];
  const vtkIdType* aZ = weights->Positions[2];

  const F* fX = static_cast<const F*>(weights->Weights[0]);
  const F* fY = static_cast<const F*>(weights->Weights[1]);
  const F* fZ = static_cast<const F*>(weights->Weights[2]);

  if (kernelSizeX == 1 && kernelSizeY == 1 && kernelSizeZ == 1)
  {
    // no interpolation, direct input to output
    aX += idX * kernelSizeX;
    aY += idY * kernelSizeY;
    aZ += idZ * kernelSizeZ;
    inPtr += *aY + *aZ;

    for (int j = 0; j < pixelCount; j++)
    {
      const T* tmpPtr = inPtr + (*aX++);
      for (int i = 0; i < ncomp; i++)
      {
        *outPtr++ = *tmpPtr++;
      }
    }
  }
  else if (kernelSizeZ == 1)
  {
    // it is possible to just apply a 2D filter to each slice
    aX += extent[0] * kernelSizeX;
    aY += idY * kernelSizeY;
    aZ += idZ * kernelSizeZ;
    fX += extent[0] * kernelSizeX;
    fY += idY * kernelSizeY;

    vtkSlidingWindow2D(&inPtr[*aZ], outPtr, extent, idX, idY, lastIdY, pixelCount, ncomp, aX, fX,
      kernelSizeX, aY, fY, kernelSizeY, workPtr);
  }
  else
  {
    // apply filter in all three directions
    F** slicePtr = workPtr + kernelSizeY;
    int extentX = extent[1] - extent[0] + 1;
    vtkIdType outIncX = ncomp;
    vtkIdType rowSize = extentX * ncomp;

    aX += extent[0] * kernelSizeX;
    aY += extent[2] * kernelSizeY;
    aZ += idZ * kernelSizeZ;
    fX += extent[0] * kernelSizeX;
    fY += extent[2] * kernelSizeY;
    fZ += idZ * kernelSizeZ;

    // loop through the XY slices
    if (idZ != lastIdZ)
    {
      // look for overlap between the slices that are currently stored and
      // the slices that will be needed for this iteration
      int m = 0;
      if (lastIdZ >= extent[4])
      {
        const vtkIdType* lZ = aZ - (idZ - lastIdZ) * kernelSizeZ;
        for (int j = 0; j < kernelSizeZ; j++)
        {
          const vtkIdType* bZ = lZ + j;
          const vtkIdType* cZ = aZ;
          int i = kernelSizeZ - j;
          do
          {
            if (*cZ++ != *bZ++)
            {
              break;
            }
          } while (--i);
          if (i == 0)
          {
            m = kernelSizeZ - j;
            break;
          }
        }
      }

      // reuse m of the temporary slices
      if (m < kernelSizeZ)
      {
        for (int i = 0; i < m; i++)
        {
          int j = kernelSizeZ + i - m;
          F* tmpPtr = slicePtr[i];
          slicePtr[i] = slicePtr[j];
          slicePtr[j] = tmpPtr;
        }

        // compute the new slices that are needed
        int extentY = extent[3] - extent[2] + 1;
        for (int i = m; i < kernelSizeZ; i++)
        {
          for (int dY = 0; dY < extentY; dY++)
          {
            vtkSlidingWindow2D(&inPtr[aZ[i]], slicePtr[i] + dY * rowSize, extent, extent[0],
              extent[2] + dY, extent[2] + dY - 1, extentX, ncomp, aX, fX, kernelSizeX,
              aY + dY * kernelSizeY, fY + dY * kernelSizeY, kernelSizeY, workPtr);
          }
        }
      }
    }

    // get one row of output data at the current Y,Z position
    vtkIdType begin = (idY - extent[2]) * rowSize + (idX - extent[0]) * outIncX;
    vtkIdType end = begin + pixelCount * outIncX;
    vtkSlidingWindowYOrZ(slicePtr, outPtr, begin, end, fZ, kernelSizeZ);
  }
}

//----------------------------------------------------------------------------
// get row interpolation function for different interpolation modes
// and different scalar types
template <class F>
void vtkSlidingWindowGetRowInterpolationFunc(
  void (**summation)(vtkInterpolationWeights* weights, int idX, int idY, int idZ, F* outPtr, int n),
  int scalarType)
{
  switch (scalarType)
  {
    vtkTemplateAliasMacro(*summation = &(vtkSlidingWindow<F, VTK_TT>::InterpolateRow));
    default:
      *summation = nullptr;
  }
}

} // namespace

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::Update()
{
  vtkDataArray* scalars = this->Scalars;

  // check for scalars
  if (!scalars)
  {
    this->InterpolationInfo->Pointer = nullptr;
    this->InterpolationInfo->NumberOfComponents = 1;

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
  void* inPtr = scalars->GetVoidPointer(0);
  info->Pointer = static_cast<char*>(inPtr) + component * dataSize;

  // set all other elements of the InterpolationInfo
  info->ScalarType = scalars->GetDataType();
  info->NumberOfComponents = this->ComputeNumberOfComponents(ncomp);
  info->BorderMode = this->BorderMode;

  // subclass-specific update
  this->InternalUpdate();

  // get the functions that will perform the interpolation
  this->GetInterpolationFunc(&this->InterpolationFuncDouble);
  this->GetInterpolationFunc(&this->InterpolationFuncFloat);

  if (this->SlidingWindow)
  {
    this->GetSlidingWindowFunc(&this->RowInterpolationFuncDouble);
    this->GetSlidingWindowFunc(&this->RowInterpolationFuncFloat);
  }
  else
  {
    this->GetRowInterpolationFunc(&this->RowInterpolationFuncDouble);
    this->GetRowInterpolationFunc(&this->RowInterpolationFuncFloat);
  }
}

//----------------------------------------------------------------------------
bool vtkAbstractImageInterpolator::Interpolate(const double point[3], double* value)
{
  double p[3];
  p[0] = (point[0] - this->Origin[0]) / this->Spacing[0];
  p[1] = (point[1] - this->Origin[1]) / this->Spacing[1];
  p[2] = (point[2] - this->Origin[2]) / this->Spacing[2];

  if (this->CheckBoundsIJK(p))
  {
    this->InterpolationFuncDouble(this->InterpolationInfo, p, value);
    return true;
  }

  int n = this->InterpolationInfo->NumberOfComponents;
  for (int i = 0; i < n; i++)
  {
    value[i] = this->OutValue;
  }
  return false;
}

//----------------------------------------------------------------------------
double vtkAbstractImageInterpolator::Interpolate(double x, double y, double z, int component)
{
  double value = this->OutValue;
  double point[3];
  point[0] = x;
  point[1] = y;
  point[2] = z;

  double p[3];
  p[0] = (point[0] - this->Origin[0]) / this->Spacing[0];
  p[1] = (point[1] - this->Origin[1]) / this->Spacing[1];
  p[2] = (point[2] - this->Origin[2]) / this->Spacing[2];

  if (this->CheckBoundsIJK(p))
  {
    vtkInterpolationInfo iinfo(*this->InterpolationInfo);
    int ncomp = static_cast<int>(iinfo.Increments[0]);
    ncomp -= this->ComponentOffset;
    int size = vtkAbstractArray::GetDataTypeSize(iinfo.ScalarType);

    component = ((component > 0) ? component : 0);
    component = ((component < ncomp) ? component : ncomp - 1);

    iinfo.Pointer = (static_cast<const char*>(iinfo.Pointer) + size * component);
    iinfo.NumberOfComponents = 1;

    this->InterpolationFuncDouble(&iinfo, p, &value);
  }

  return value;
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::GetInterpolationFunc(
  void (**)(vtkInterpolationInfo*, const double[3], double*))
{
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::GetInterpolationFunc(
  void (**)(vtkInterpolationInfo*, const float[3], float*))
{
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::GetRowInterpolationFunc(
  void (**)(vtkInterpolationWeights*, int, int, int, double*, int))
{
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::GetRowInterpolationFunc(
  void (**)(vtkInterpolationWeights*, int, int, int, float*, int))
{
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::GetSlidingWindowFunc(
  void (**)(vtkInterpolationWeights*, int, int, int, double*, int))
{
  vtkSlidingWindowGetRowInterpolationFunc(
    &this->RowInterpolationFuncDouble, this->InterpolationInfo->ScalarType);
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::GetSlidingWindowFunc(
  void (**)(vtkInterpolationWeights*, int, int, int, float*, int))
{
  vtkSlidingWindowGetRowInterpolationFunc(
    &this->RowInterpolationFuncFloat, this->InterpolationInfo->ScalarType);
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::PrecomputeWeightsForExtent(
  const double[16], const int[6], int[6], vtkInterpolationWeights*&)
{
  vtkErrorMacro("PrecomputeWeights not supported for this interpolator");
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::PrecomputeWeightsForExtent(
  const float[16], const int[6], int[6], vtkInterpolationWeights*&)
{
  vtkErrorMacro("PrecomputeWeights not supported for this interpolator");
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::FreePrecomputedWeights(vtkInterpolationWeights*& weights)
{
  int* extent = weights->WeightExtent;

  for (int k = 0; k < 3; k++)
  {
    int step = weights->KernelSize[k];
    weights->Positions[k] += step * extent[2 * k];
    delete[] weights->Positions[k];
    if (weights->Weights[k])
    {
      if (weights->WeightType == VTK_FLOAT)
      {
        float* constants = static_cast<float*>(weights->Weights[k]);
        constants += step * extent[2 * k];
        delete[] constants;
      }
      else
      {
        double* constants = static_cast<double*>(weights->Weights[k]);
        constants += step * extent[2 * k];
        delete[] constants;
      }
    }
  }

  if (weights->Workspace)
  {
    if (weights->WeightType == VTK_FLOAT)
    {
      float** workPtr = static_cast<float**>(weights->Workspace);
      float* firstPtr = workPtr[0];
      for (int i = 1; i < weights->KernelSize[1]; i++)
      {
        firstPtr = (workPtr[i] < firstPtr ? workPtr[i] : firstPtr);
      }
      delete[] firstPtr;
      delete[] workPtr;
    }
    else
    {
      double** workPtr = static_cast<double**>(weights->Workspace);
      double* firstPtr = workPtr[0];
      for (int i = 1; i < weights->KernelSize[1]; i++)
      {
        firstPtr = (workPtr[i] < firstPtr ? workPtr[i] : firstPtr);
      }
      delete[] firstPtr;
      delete[] workPtr;
    }
  }

  delete weights;

  weights = nullptr;
}

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int* vtkAbstractImageInterpolator::GetWholeExtent()
{
  VTK_LEGACY_REPLACED_BODY(vtkAbstractImageInterpolator::GetWholeExtent, "VTK 7.1",
    vtkAbstractImageInterpolator::GetExtent);
  return this->GetExtent();
}

void vtkAbstractImageInterpolator::GetWholeExtent(int extent[6])
{
  VTK_LEGACY_REPLACED_BODY(vtkAbstractImageInterpolator::GetWholeExtent, "VTK 7.1",
    vtkAbstractImageInterpolator::GetExtent);
  this->GetExtent(extent);
}
#endif
