/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResize.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageResize.h"

#include "vtkImageInterpolator.h"
#include "vtkImageSincInterpolator.h"
#include "vtkImageInterpolatorInternals.h"
#include "vtkImageData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkObjectFactory.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkMath.h"

#include "vtkTemplateAliasMacro.h"
// turn off 64-bit ints when templating over all types
# undef VTK_USE_INT64
# define VTK_USE_INT64 0
# undef VTK_USE_UINT64
# define VTK_USE_UINT64 0

#include <math.h>

vtkStandardNewMacro(vtkImageResize);
vtkCxxSetObjectMacro(vtkImageResize,Interpolator,vtkAbstractImageInterpolator);

//----------------------------------------------------------------------------
vtkImageResize::vtkImageResize()
{
  this->ResizeMethod = vtkImageResize::OUTPUT_DIMENSIONS;

  this->OutputDimensions[0] = -1;
  this->OutputDimensions[1] = -1;
  this->OutputDimensions[2] = -1;

  this->OutputSpacing[0] = 0;
  this->OutputSpacing[1] = 0;
  this->OutputSpacing[2] = 0;

  this->MagnificationFactors[0] = 1.0;
  this->MagnificationFactors[1] = 1.0;
  this->MagnificationFactors[2] = 1.0;

  this->Border = 0;
  this->Cropping = 0;

  this->CroppingRegion[0] = 0.0;
  this->CroppingRegion[1] = 1.0;
  this->CroppingRegion[2] = 0.0;
  this->CroppingRegion[3] = 1.0;
  this->CroppingRegion[4] = 0.0;
  this->CroppingRegion[5] = 1.0;

  this->IndexStretch[0] = 1.0;
  this->IndexStretch[1] = 1.0;
  this->IndexStretch[2] = 1.0;

  this->IndexTranslate[0] = 0.0;
  this->IndexTranslate[1] = 0.0;
  this->IndexTranslate[2] = 0.0;

  this->Interpolator = NULL;
  this->NNInterpolator = NULL;
  this->Interpolate = 1;
}

//----------------------------------------------------------------------------
vtkImageResize::~vtkImageResize()
{
  this->SetInterpolator(NULL);
  if (this->NNInterpolator)
    {
    this->NNInterpolator->Delete();
    }
}

//----------------------------------------------------------------------------
const char *vtkImageResize::GetResizeMethodAsString()
{
  switch (this->ResizeMethod)
    {
    case vtkImageResize::OUTPUT_DIMENSIONS:
      return "OutputDimensions";
    case vtkImageResize::OUTPUT_SPACING:
      return "OutputSpacing";
    case vtkImageResize::MAGNIFICATION_FACTORS:
      return "MagnificationFactors";
    }
  return "";
}

//----------------------------------------------------------------------------
int vtkImageResize::RequestInformation(
  vtkInformation *, vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  int inExt[6], outExt[6];
  double inSpacing[3], outSpacing[3];
  double inOrigin[3], outOrigin[3];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
  inInfo->Get(vtkDataObject::SPACING(), inSpacing);
  inInfo->Get(vtkDataObject::ORIGIN(), inOrigin);

  int inDims[3], outDims[3];
  inDims[0] = (inExt[1] - inExt[0] + 1);
  inDims[1] = (inExt[3] - inExt[2] + 1);
  inDims[2] = (inExt[5] - inExt[4] + 1);

  // extend image bounds by half a voxel
  double b = ((this->Border != 0) ? 0.5 : 0.0);

  double bounds[6];
  for (int j = 0; j < 3; j++)
    {
    bounds[2*j] = inExt[2*j] - b;
    bounds[2*j+1] = inExt[2*j+1] + b;
    outExt[2*j] = inExt[2*j];
    outSpacing[j] = inSpacing[j];
    outOrigin[j] = inOrigin[j];
    outDims[j] = inDims[j];
    }

  if (this->Cropping)
    {
    this->GetCroppingRegion(bounds);
    for (int k = 0; k < 3; k++)
      {
      // if bounds are reversed
      if (bounds[2*k] > bounds[2*k+1])
        {
        double tmp = bounds[2*k];
        bounds[2*k] = bounds[2*k+1];
        bounds[2*k+1] = tmp;
        }
      double l = (bounds[2*k] - inOrigin[k])/inSpacing[k];
      double h = (bounds[2*k+1] - inOrigin[k])/inSpacing[k];
      int flip = (inSpacing[k] < 0);
      bounds[2*k+flip] = l;
      bounds[2*k+1-flip] = h;
      }
    }

  switch (this->ResizeMethod)
    {
    case vtkImageResize::OUTPUT_DIMENSIONS:
      {
      for (int i = 0; i < 3; i++)
        {
        if (this->OutputDimensions[i] > 0)
          {
          outDims[i] = this->OutputDimensions[i];
          }
        double d = (outDims[i] - 1) + 2*b;
        double e = bounds[2*i+1] - bounds[2*i];
        this->IndexStretch[i] = 1.0;
        if (d != 0 && e != 0)
          {
          this->IndexStretch[i] *= e/d;
          }
        int flip = (this->IndexStretch[i] < 0);
        this->IndexTranslate[i] =
          (bounds[2*i + flip] - (outExt[2*i] - b)*this->IndexStretch[i]);

        outSpacing[i] = inSpacing[i]*this->IndexStretch[i];
        outOrigin[i] = inOrigin[i] + inSpacing[i]*this->IndexTranslate[i];
        }
      }
      break;
    case vtkImageResize::OUTPUT_SPACING:
      {
      for (int i = 0; i < 3; i++)
        {
        if (this->OutputSpacing[i] != 0)
          {
          outSpacing[i] = this->OutputSpacing[i];
          }
        this->IndexStretch[i] = outSpacing[i]/inSpacing[i];
        int flip = (this->IndexStretch[i] < 0);
        this->IndexTranslate[i] =
          (bounds[2*i + flip] - (outExt[2*i] - b)*this->IndexStretch[i]);

        outOrigin[i] = inOrigin[i] + inSpacing[i]*this->IndexTranslate[i];

        double e = bounds[2*i+1] - bounds[2*i];
        double d = fabs(e/this->IndexStretch[i]) - 2*b;
        outDims[i] = static_cast<int>(d + VTK_INTERPOLATE_FLOOR_TOL) + 1;
        }
      }
      break;
    case vtkImageResize::MAGNIFICATION_FACTORS:
      {
      for (int i = 0; i < 3; i++)
        {
        this->IndexStretch[i] = 1.0;
        if (this->MagnificationFactors[i] != 0)
          {
          this->IndexStretch[i] /= this->MagnificationFactors[i];
          outSpacing[i] = inSpacing[i]/this->MagnificationFactors[i];
          }
        int flip = (this->IndexStretch[i] < 0);
        this->IndexTranslate[i] =
          (bounds[2*i + flip] - (outExt[2*i] - b)*this->IndexStretch[i]);

        outOrigin[i] = inOrigin[i] + inSpacing[i]*this->IndexTranslate[i];

        double e = bounds[2*i+1] - bounds[2*i];
        double d = fabs(e/this->IndexStretch[i]) - 2*b;
        outDims[i] = static_cast<int>(d + VTK_INTERPOLATE_FLOOR_TOL) + 1;
        }
      }
      break;
    }

  for (int k = 0; k < 3; k++)
    {
    outExt[2*k+1] = outExt[2*k] + outDims[k] - 1;
    }

  // set the output information
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), outExt, 6);
  outInfo->Set(vtkDataObject::SPACING(), outSpacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), outOrigin, 3);

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageResize::RequestUpdateExtent(
  vtkInformation *, vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  int wholeExt[6];
  int extent[6];

  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);

  // get the interpolator
  vtkAbstractImageInterpolator *interpolator = this->GetInternalInterpolator();

  // set the extent according to the interpolation kernel size:
  // first create a matrix to map output to input indices
  double elements[16];
  for (int i = 0; i < 3; i++)
    {
    elements[4*i+0] = elements[4*i+1] = elements[4*i+2] = 0;
    elements[4*i+i] = this->IndexStretch[i];
    elements[4*i+3] = this->IndexTranslate[i];
    elements[12+i] = 0;
    }
  elements[15] = 1;
  // get the kernel size
  int supportSize[3];
  interpolator->ComputeSupportSize(elements, supportSize);

  for (int j = 0; j < 3; j++)
    {
    double range[2];
    range[0] = extent[2*j]*this->IndexStretch[j] + this->IndexTranslate[j];
    range[1] = extent[2*j+1]*this->IndexStretch[j]+this->IndexTranslate[j];

    extent[2*j] = VTK_INT_MAX;
    extent[2*j+1] = VTK_INT_MIN;

    for (int ii = 0; ii < 2; ii++)
      {
      int kernelSize = supportSize[j];
      int extra = (kernelSize + 1)/2 - 1;

      // most kernels have even size
      if ((kernelSize & 1) == 0)
        {
        double f;
        int k = vtkInterpolationMath::Floor(range[ii], f);
        if (k - extra < extent[2*j])
          {
          extent[2*j] = k - extra;
          }
        k += (f != 0);
        if (k + extra > extent[2*j+1])
          {
          extent[2*j+1] = k + extra;
          }
        }
      // else is for kernels with odd size
      else
        {
        int k = vtkInterpolationMath::Round(range[ii]);
        if (k < extent[2*j])
          {
          extent[2*j] = k - extra;
          }
        if (k > extent[2*j+1])
          {
          extent[2*j+1] = k + extra;
          }
        }
      }

    if (extent[2*j] < wholeExt[2*j])
      {
      extent[2*j] = wholeExt[2*j];
      }
    if (extent[2*j+1] > wholeExt[2*j+1])
      {
      extent[2*j+1] = wholeExt[2*j+1];
      }
    }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);

  return 1;
}

//----------------------------------------------------------------------------
// Methods used by execute

namespace {

#define VTK_RESIZE_CONVERT_INT_CLAMP(T, minval, maxval) \
void vtkImageResizeConvert(double v, T &u) \
{ \
  static double vmin = minval; \
  static double vmax = maxval; \
  v = (v > vmin ? v : vmin); \
  v = (v < vmax ? v : vmax); \
  u = vtkInterpolationMath::Round(v); \
}

#define VTK_RESIZE_CONVERT_FLOAT(T) \
void vtkImageResizeConvert(double v, T &u) \
{ \
  u = static_cast<T>(v); \
}

VTK_RESIZE_CONVERT_INT_CLAMP(vtkTypeUInt8, 0.0, 255.0);
VTK_RESIZE_CONVERT_INT_CLAMP(vtkTypeUInt16, 0.0, 65535.0);
VTK_RESIZE_CONVERT_INT_CLAMP(vtkTypeUInt32, 0.0, 4294967295.0);
VTK_RESIZE_CONVERT_INT_CLAMP(vtkTypeInt8, -128.0, 127.0);
VTK_RESIZE_CONVERT_INT_CLAMP(vtkTypeInt16, -32768.0, 32767.0);
VTK_RESIZE_CONVERT_INT_CLAMP(vtkTypeInt32, -2147483648.0, 2147483647.0);
VTK_RESIZE_CONVERT_FLOAT(vtkTypeFloat32);
VTK_RESIZE_CONVERT_FLOAT(vtkTypeFloat64);

//----------------------------------------------------------------------------
// Apply a 1D filter in the X direction.
// The inPtr parameter must be positioned at the correct slice.
template<class T>
void vtkImageResizeFilterX(
  const T *inPtr, double *outPtr, int ncomp, const int extent[6],
  const vtkIdType *a, const double *f, int kernelSize)
{
  int pixelCounter = extent[1] - extent[0] + 1;

  if (kernelSize == 1)
    {
    do
      {
      const T *tmpPtr = inPtr + (*a++);
      int i = ncomp;
      do
        {
        *outPtr++ = *tmpPtr++;
        }
      while (--i);
      }
    while (--pixelCounter);
    }
  else
    {
    do
      {
      const T *tmpPtr = inPtr;
      int i = ncomp;
      do
        {
        const vtkIdType *b = a;
        const double *g = f;
        double val = (*g++)*tmpPtr[*b++];
        int k = kernelSize - 1;
        do
          {
          val += (*g++)*tmpPtr[*b++];
          }
        while (--k);
        tmpPtr++;
        vtkImageResizeConvert(val, *outPtr);
        outPtr++;
        }
      while (--i);
      a += kernelSize;
      f += kernelSize;
      }
    while (--pixelCounter);
    }
}

//----------------------------------------------------------------------------
// Apply a 1D filter along the Y or Z direction, given kernelSize rows
// of data as input and producing one row of data as output.  This function
// must be called for each row of the output to filter a whole slice.
template<class T>
void vtkImageResizeFilterYOrZ(
  double **rowPtr, T *outPtr, int ncomp, const int extent[6],
  const double *f, int kernelSize)
{
  // number of data values in one row
  vtkIdType rowCounter = (extent[1] - extent[0] + 1)*ncomp;

  if (kernelSize == 1)
    {
    // don't apply the filter, just convert the data
    double *tmpPtr = *rowPtr;
    do
      {
      vtkImageResizeConvert(*tmpPtr, *outPtr);
      outPtr++;
      tmpPtr++;
      }
    while (--rowCounter);
    }
  else
    {
    // apply the filter to one row of the image
    int i = 0;
    do
      {
      double **tmpPtr = rowPtr;
      const double *g = f;
      double val = (*g++)*((*tmpPtr++)[i]);
      int k = kernelSize - 1;
      do
        {
        val += (*g++)*((*tmpPtr++)[i]);
        }
      while (--k);
      i++;
      vtkImageResizeConvert(val, *outPtr);
      outPtr++;
      }
    while (--rowCounter);
    }
}

//----------------------------------------------------------------------------
// Apply a 2D filter to image slices (either XY or XZ slices).
// The inPtr parameter must be positioned at the correct slice.
template<class T, class U>
void vtkImageResizeFilter2D(
  const T *inPtr, U *outPtr, const vtkIdType outInc[3], const int extent[6],
  const vtkIdType *aX, const double *fX, int kernelSizeX,
  const vtkIdType *a, const double *f, int kernelSize,
  double **workPtr, int direction, vtkAlgorithm *progress)
{
  int ncomp = static_cast<int>(outInc[0]);
  int idYMin = extent[2*direction];
  int idYMax = extent[2*direction+1];

  int progressGoal = idYMax - idYMin + 1;
  int progressStep = (progressGoal + 49)/50;
  int progressCount = 0;

  if (kernelSize == 1)
    {
    // filter only in the X direction
    for (int idY = idYMin; idY <= idYMax; idY++)
      {
      if (progress != NULL && (progressCount % progressStep) == 0)
        {
        progress->UpdateProgress(progressCount*1.0/progressGoal);
        }
      progressCount++;

      vtkImageResizeFilterX(
        &inPtr[*a], *workPtr, ncomp, extent, aX, fX, kernelSizeX);

      vtkImageResizeFilterYOrZ(
        workPtr, outPtr, ncomp, extent, f, kernelSize);

      outPtr += outInc[direction];
      a += kernelSize;
      f += kernelSize;
      }
    }
  else
    {
    // filter in both X and Y directions
    int j = kernelSize;
    for (int idY = idYMin; idY <= idYMax; idY++)
      {
      if (progress != NULL && (progressCount % progressStep) == 0)
        {
        progress->UpdateProgress(progressCount*1.0/progressGoal);
        }
      progressCount++;

      // rotate workspace rows to reuse the ones that can be reused
      for (int k = 0; k < kernelSize-j; k++)
        {
        double *tmpPtr = workPtr[k];
        workPtr[k] = workPtr[k+j];
        workPtr[k+j] = tmpPtr;
        }

      // compute the j new rows that must be computed
      if (j) do
        {
        vtkImageResizeFilterX(
          &inPtr[*a], workPtr[kernelSize-j], ncomp, extent, aX, fX,
          kernelSizeX);

        a++;
        }
      while (--j);

      // if this is not the final iteration, then look for overlap between
      // the rows that are currently stored and the rows that will be
      // needed for the next iteration, store the number of new rows that
      // will be needed in variable "j" for use in the next iteration
      if (idY < idYMax)
        {
        for (j = 0; j < kernelSize; j++)
          {
          int i = kernelSize - j;
          const vtkIdType *b = a - i;
          const vtkIdType *c = a;
          do
            {
            if (*c++ != *b++) { break; }
            }
          while (--i);
          if (i == 0) { break; }
          }
        a += kernelSize-j;
        }

      vtkImageResizeFilterYOrZ(
        workPtr, outPtr, ncomp, extent, f, kernelSize);

      outPtr += outInc[direction];
      f += kernelSize;
      }
    }
}

//----------------------------------------------------------------------------
// Apply separable blur filter fX, fY, fZ to an image with minimum
// memory overhead (3 rows of temp storage for 2D, 3 slices for 3D).
// The aX, aY, and aZ contain increments for the X, Y, and Z
// directions.
template<class T>
void vtkImageResizeFilter3D(
  const T *inPtr, T *outPtr, const vtkIdType outInc[3], const int extent[6],
  const vtkIdType *aX, const double *fX, int kernelSizeX,
  const vtkIdType *aY, const double *fY, int kernelSizeY,
  const vtkIdType *aZ, const double *fZ, int kernelSizeZ,
  vtkAlgorithm *progress)
{
  vtkIdType rowSize = outInc[0]*(extent[1] - extent[0] + 1);
  int ncomp = static_cast<int>(outInc[0]);

  aX += extent[0]*kernelSizeX;
  aY += extent[2]*kernelSizeY;
  aZ += extent[4]*kernelSizeZ;

  fX += extent[0]*kernelSizeX;
  fY += extent[2]*kernelSizeY;
  fZ += extent[4]*kernelSizeZ;

  if (kernelSizeX == 1 && kernelSizeY == 1 && kernelSizeZ == 1)
    {
    // no interpolation, no intermediate data needed
    int idYMin = extent[2];
    int idYMax = extent[3];
    int idZMin = extent[4];
    int idZMax = extent[5];
    int pixelCounter = extent[1] - extent[0] + 1;

    vtkIdType progressGoal = (idYMax - idYMin + 1);
    progressGoal *= (idZMax - idZMin + 1);
    vtkIdType progressCount = 0;
    vtkIdType progressStep = (progressGoal + 49)/50;

    if (pixelCounter > 0)
      {
      for (int idZ = idZMin; idZ <= idZMax; idZ++)
        {
        const T *tmpPtrZ = inPtr + (*aZ++);
        const vtkIdType *aYtmp = aY;
        for (int idY = idYMin; idY <= idYMax; idY++)
          {
          const T *tmpPtrY = tmpPtrZ + (*aYtmp++);
          const vtkIdType *aXtmp = aX;

          if (progress != NULL && (progressCount % progressStep) == 0)
            {
            progress->UpdateProgress(progressCount*1.0/progressGoal);
            }
          progressCount++;

          int j = pixelCounter;
          do
            {
            const T *tmpPtr = tmpPtrY + (*aXtmp++);
            int i = ncomp;
            do
              {
              *outPtr++ = *tmpPtr++;
              }
            while (--i);
            }
          while (--j);
          }
        }
      }
    }
  else if (kernelSizeZ == 1 || kernelSizeY == 1)
    {
    // it is possible to just apply a 2D filter to each slice
    int sliceDirection = 2;
    int direction = 1;
    int kernelSizeSlice = kernelSizeZ;
    int kernelSize = kernelSizeY;
    const vtkIdType *aSlice = aZ;
    const vtkIdType *a = aY;
    const double *f = fY;
    if (kernelSizeY == 1)
      {
      sliceDirection = 1;
      direction = 2;
      kernelSizeSlice = kernelSizeY;
      kernelSize = kernelSizeZ;
      aSlice = aY;
      a = aZ;
      f = fZ;
      }

    // apply filter to all XY or XZ slices
    vtkIdType workSize = rowSize*kernelSize;
    double *workPtr2 = new double[workSize];
    double **workPtr = new double *[kernelSize];
    for (int ii = 0; ii < kernelSize; ii++)
      {
      workPtr[ii] = workPtr2 + ii*rowSize;
      }

    // the slice range
    int sliceMin = extent[2*sliceDirection];
    int sliceMax = extent[2*sliceDirection+1];

    // progress reporting variables
    int progressGoal = sliceMax - sliceMin + 1;
    int progressStep = (progressGoal + 49)/50;
    int progressCount = 0;
    vtkAlgorithm *rowProgress = NULL;
    if (progressGoal == 1)
      {
      // if one slice, report progress by rows instead
      rowProgress = progress;
      progress = NULL;
      }

    for (int slice = sliceMin; slice <= sliceMax; slice++)
      {
      if (progress != NULL && (progressCount % progressStep) == 0)
        {
        progress->UpdateProgress(progressCount*1.0/progressGoal);
        }
      progressCount++;

      vtkImageResizeFilter2D(
        &inPtr[*aSlice], outPtr, outInc, extent,
        aX, fX, kernelSizeX, a, f, kernelSize,
        workPtr, direction, rowProgress);

      aSlice += kernelSizeSlice;
      outPtr += outInc[sliceDirection];
      }

    delete [] workPtr2;
    delete [] workPtr;
    }
  else
    {
    // apply filter in all three directions: first X, then Z, then Y
    // (doing Z second is most efficient, memory-wise, because it is
    // the dimension broken up between threads)

    // compute temporary workspace requirements
    vtkIdType sliceSize = rowSize*(extent[5] - extent[4] + 1);
    vtkIdType workSize = rowSize*kernelSizeZ;
    workSize += sliceSize*kernelSizeY;

    // part of workspace goes to temporary rows
    double *workPtr2 = new double[workSize];
    double **workPtr = new double *[kernelSizeZ + kernelSizeY];
    for (int jj = 0; jj < kernelSizeZ; jj++)
      {
      workPtr[jj] = workPtr2 + jj*rowSize;
      }

    // part of the workspace goes to temporary slices
    double *workPtr3 = workPtr2 + kernelSizeZ*rowSize;
    double **slicePtr = workPtr + kernelSizeZ;
    for (int ii = 0; ii < kernelSizeY; ii++)
      {
      slicePtr[ii] = workPtr3 + ii*sliceSize;
      }

    // increments for temporary slices
    vtkIdType sliceInc[3];
    sliceInc[0] = outInc[0];
    sliceInc[1] = sliceInc[0]*(extent[1] - extent[0] + 1);
    sliceInc[2] = sliceInc[1];

    // progress reporting variables
    int progressGoal = extent[3] - extent[2] + 1;
    int progressStep = (progressGoal + 49)/50;
    int progressCount = 0;

    // loop through the XZ slices
    int j = kernelSizeY;
    for (int idY = extent[2]; idY <= extent[3]; idY++)
      {
      if (progress != NULL && (progressCount % progressStep) == 0)
        {
        progress->UpdateProgress(progressCount*1.0/progressGoal);
        }
      progressCount++;

      // reuse all but j of the temporary slices
      for (int k = 0; k < kernelSizeY-j; k++)
        {
        double *tmpPtr = slicePtr[k];
        slicePtr[k] = slicePtr[k+j];
        slicePtr[k+j] = tmpPtr;
        }

      // compute the j new slices that are needed
      if (j) do
        {
        vtkImageResizeFilter2D(
          &inPtr[*aY], slicePtr[kernelSizeY-j], sliceInc, extent,
          aX, fX, kernelSizeX, aZ, fZ, kernelSizeZ,
          workPtr, 2, NULL);

        aY++;
        }
      while (--j);

      // if this is not the final iteration, then look for overlap between
      // the slices that are currently stored and the slices that will be
      // needed for the next iteration, store the number of new slices that
      // will be needed in variable "j" for use in the next iteration
      if (idY < extent[3])
        {
        for (j = 0; j < kernelSizeY; j++)
          {
          int i = kernelSizeY - j;
          const vtkIdType *bY = aY - i;
          const vtkIdType *cY = aY;
          do
            {
            if (*cY++ != *bY++) { break; }
            }
          while (--i);
          if (i == 0) { break; }
          }
        aY += kernelSizeY-j;
        }

      // loop through the rows of this slice
      T *outPtr0 = outPtr;
      for (int idZ = extent[4]; idZ <= extent[5]; idZ++)
        {
        vtkImageResizeFilterYOrZ(
          slicePtr, outPtr0, ncomp, extent, fY, kernelSizeY);

        outPtr0 += outInc[2];
        for (int i = 0; i < kernelSizeY; i++)
          {
          slicePtr[i] += rowSize;
          }
        }

      // reset the slicePtr values to their initial values
      for (int i = 0; i < kernelSizeY; i++)
        {
        slicePtr[i] -= sliceSize;
        }

      fY += kernelSizeY;
      outPtr += outInc[1];
      }

    delete [] workPtr2;
    delete [] workPtr;
    }
}

} // end anonymous namespace

//----------------------------------------------------------------------------
// RequestData is where the interpolator is updated, since it must be updated
// before the threads are split
int vtkImageResize::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkAbstractImageInterpolator *interpolator = this->GetInternalInterpolator();
  vtkInformation* info = inputVector[0]->GetInformationObject(0);
  interpolator->Initialize(info->Get(vtkDataObject::DATA_OBJECT()));

  int rval = this->Superclass::RequestData(request, inputVector, outputVector);

  interpolator->ReleaseData();

  return rval;
}

//----------------------------------------------------------------------------
void vtkImageResize::ThreadedRequestData(vtkInformation *,
  vtkInformationVector **, vtkInformationVector *,
  vtkImageData ***, vtkImageData **outData, int extent[6],
  int threadId)
{
  vtkDebugMacro("Execute: outData = " << outData);

  // get the pointer and increments
  vtkIdType outInc[3];
  outData[0]->GetIncrements(outInc);
  void *outPtr = outData[0]->GetScalarPointerForExtent(extent);
  int outScalarType = outData[0]->GetScalarType();

  // create a matrix to map output to input indices
  double newmat[4][4];
  for (int i = 0; i < 3; i++)
    {
    newmat[i][0] = newmat[i][1] = newmat[i][2] = 0;
    newmat[i][i] = this->IndexStretch[i];
    newmat[i][3] = this->IndexTranslate[i];
    newmat[3][i] = 0;
    }
  newmat[3][3] = 1;

  // fill in the interpolation tables
  vtkAbstractImageInterpolator *interpolator = this->GetInternalInterpolator();
  int clipExt[6];
  vtkInterpolationWeights *weights;
  interpolator->PrecomputeWeightsForExtent(*newmat, extent, clipExt, weights);

  // prepare table for use by this filter
  int kernelSizeX = weights->KernelSize[0];
  vtkIdType *aX = weights->Positions[0];
  const double *fX = static_cast<double *>(weights->Weights[0]);
  int kernelSizeY = weights->KernelSize[1];
  vtkIdType *aY = weights->Positions[1];
  const double *fY = static_cast<double *>(weights->Weights[1]);
  int kernelSizeZ = weights->KernelSize[2];
  vtkIdType *aZ = weights->Positions[2];
  const double *fZ = static_cast<double *>(weights->Weights[2]);

  // get the pointer and scalar type
  const void *inPtr = weights->Pointer;
  int inScalarType = weights->ScalarType;

  // progress object if main thread
  vtkAlgorithm *progress = ((threadId == 0) ? this : NULL);

  // call the execute method
  if (outScalarType == inScalarType)
    {
    switch (inScalarType)
      {
      vtkTemplateAliasMacro(
        vtkImageResizeFilter3D(
          static_cast<const VTK_TT *>(inPtr),
          static_cast<VTK_TT *>(outPtr), outInc, extent,
          aX, fX, kernelSizeX,
          aY, fY, kernelSizeY,
          aZ, fZ, kernelSizeZ,
          progress));
      default:
        vtkErrorMacro("Execute: Unknown ScalarType");
      }
    }
  else
    {
    vtkErrorMacro("ThreadedRequestData: output scalar type does not match "
                  "input scalar type");
    }

  interpolator->FreePrecomputedWeights(weights);
}

//----------------------------------------------------------------------------
void vtkImageResize::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ResizeMethod: " << this->GetResizeMethodAsString() << "\n";

  os << indent << "OutputDimensions: "
     << this->OutputDimensions[0] << " "
     << this->OutputDimensions[1] << " "
     << this->OutputDimensions[2] << "\n";

  os << indent << "OutputSpacing: "
     << this->OutputSpacing[0] << " "
     << this->OutputSpacing[1] << " "
     << this->OutputSpacing[2] << "\n";

  os << indent << "MagnificationFactors: "
     << this->MagnificationFactors[0] << " "
     << this->MagnificationFactors[1] << " "
     << this->MagnificationFactors[2] << "\n";

  os << indent << "Border: " << (this->Border ? "On\n" : "Off\n");
  os << indent << "Cropping: " << (this->Cropping ? "On\n" : "Off\n");

  os << indent << "CroppingRegion: "
     << this->CroppingRegion[0] << " " << this->CroppingRegion[1] << " "
     << this->CroppingRegion[2] << " " << this->CroppingRegion[3] << " "
     << this->CroppingRegion[4] << " " << this->CroppingRegion[5] << "\n";

  os << indent << "Interpolate: " << (this->Interpolate ? "On\n" : "Off\n");

  os << indent << "Interpolator: " << this->Interpolator << "\n";
}

//----------------------------------------------------------------------------
vtkAbstractImageInterpolator *vtkImageResize::GetInterpolator()
{
  if (this->Interpolator == NULL)
    {
    vtkImageSincInterpolator *i = vtkImageSincInterpolator::New();
    i->SetWindowFunctionToLanczos();
    i->SetWindowHalfWidth(3);
    i->AntialiasingOn();
    this->Interpolator = i;
    }

  return this->Interpolator;
}

//----------------------------------------------------------------------------
vtkAbstractImageInterpolator *vtkImageResize::GetInternalInterpolator()
{
  if (this->Interpolate)
    {
    return this->GetInterpolator();
    }

  if (!this->NNInterpolator)
    {
    vtkImageInterpolator *nn = vtkImageInterpolator::New();
    nn->SetInterpolationModeToNearest();
    this->NNInterpolator = nn;
    }

  return this->NNInterpolator;
}

//----------------------------------------------------------------------------
unsigned long int vtkImageResize::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if (this->Interpolate != 0 && this->Interpolator != NULL)
    {
    time = this->Interpolator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}
