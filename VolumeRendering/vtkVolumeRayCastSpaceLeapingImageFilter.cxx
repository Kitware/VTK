/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastSpaceLeapingImageFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeRayCastSpaceLeapingImageFilter.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include <sstream>
#include <fstream>
#include <iostream>

#ifdef vtkVolumeRayCastSpaceLeapingImageFilter_DEBUG
#include "vtkMetaImageWriter.h"
#endif

#include <math.h>

// Space leaping block size
#define VTK_SL_BLK 4

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVolumeRayCastSpaceLeapingImageFilter);
vtkCxxSetObjectMacro(vtkVolumeRayCastSpaceLeapingImageFilter,
                     CurrentScalars, vtkDataArray);

//----------------------------------------------------------------------------
vtkVolumeRayCastSpaceLeapingImageFilter::vtkVolumeRayCastSpaceLeapingImageFilter()
{
  this->ComputeMinMax = 0;
  this->ComputeGradientOpacity = 0;
  this->UpdateGradientOpacityFlags = 0;
  this->IndependentComponents  = 1;
  this->CurrentScalars = NULL;
  this->MinNonZeroScalarIndex = NULL;
  this->MinNonZeroGradientMagnitudeIndex = NULL;
  this->GradientMagnitude = NULL;
  for (int i = 0; i < 4; i++)
    {
    this->TableSize[i] = 0;
    this->TableShift[i] = 0;
    this->TableScale[i] = 1;
    this->ScalarOpacityTable[i] = NULL;
    this->GradientOpacityTable[i] = NULL;
    }
  this->Cache = NULL;
}

//----------------------------------------------------------------------------
vtkVolumeRayCastSpaceLeapingImageFilter::~vtkVolumeRayCastSpaceLeapingImageFilter()
{
  this->SetCurrentScalars(NULL);
  if (this->MinNonZeroScalarIndex)
    {
    delete [] this->MinNonZeroScalarIndex;
    this->MinNonZeroScalarIndex = NULL;
    }
  if (this->MinNonZeroGradientMagnitudeIndex)
    {
    delete [] this->MinNonZeroGradientMagnitudeIndex;
    this->MinNonZeroGradientMagnitudeIndex = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkVolumeRayCastSpaceLeapingImageFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ComputeMinMax: " << this->ComputeMinMax << "\n";
  os << indent << "ComputeGradientOpacity: " << this->ComputeGradientOpacity << "\n";
  os << indent << "UpdateGradientOpacityFlags: " << this->UpdateGradientOpacityFlags << "\n";
  os << indent << "IndependentComponents: " << this->IndependentComponents << "\n";
  os << indent << "CurrentScalars: " << this->CurrentScalars << "\n";
  // this->TableShift
  // this->TableScale
  // this->TableSize
  // this->ScalarOpacityTable
  // this->GradientOpacityTable
  // this->MinNonZeroScalarIndex
  // this->MinNonZeroGradientMagnitudeIndex
}

//----------------------------------------------------------------------------
int vtkVolumeRayCastSpaceLeapingImageFilter::RequestUpdateExtent (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // Ask for the whole input

  int wholeExtent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), wholeExtent, 6);

  return 1;
}

//----------------------------------------------------------------------------
void vtkVolumeRayCastSpaceLeapingImageFilter
::SetCache( vtkImageData * cache )
{
  // Do not reference count it to avoid reference counting loops
  this->Cache = cache;
}

//----------------------------------------------------------------------------
void vtkVolumeRayCastSpaceLeapingImageFilter
::InternalRequestUpdateExtent( int *inExt,
                               int *wholeExtent)
{
  int dim[3];

  // We group four cells (which require 5 samples) into one element in the min/max tree
  for ( int i = 0; i < 3; i++ )
    {
    // size of the input image.
    dim[i] = wholeExtent[2*i+1] - wholeExtent[2*i] + 1;

    inExt[2*i] = 0; // The output extent is 0 based.
    inExt[2*i+1] = (dim[i] < 2) ? (0) :
        (static_cast<int>((dim[i] - 2)/VTK_SL_BLK));
    }
}

//----------------------------------------------------------------------------
void
vtkVolumeRayCastSpaceLeapingImageFilterClearOutput(vtkDataArray *scalars,
                                                   vtkImageData *outData,
                                                   int outExt[6],
                                                   int nComponents )
{
  unsigned short *tmpPtr = static_cast< unsigned short * >(
                outData->GetScalarPointerForExtent(outExt));

  // Get increments to march through the thread's output extents

  vtkIdType outInc0, outInc1, outInc2;
  outData->GetContinuousIncrements(scalars,
                                   outExt, outInc0, outInc1, outInc2);

  // A. Initialize the arrays with a blank flag.

  int i,j,k;
  int c;
  for (k = outExt[4]; k <= outExt[5]; ++k, tmpPtr += outInc2)
    {
    for (j = outExt[2]; j <= outExt[3]; ++j, tmpPtr += outInc1)
      {
      for (i = outExt[0]; i <= outExt[1]; ++i)
        {
        for ( c = 0; c < nComponents; ++c )
          {
          *(tmpPtr++) = 0xffff;  // Min Scalar
          *(tmpPtr++) = 0;       // Max Scalar
          *(tmpPtr++) = 0;       // Max Gradient Magnitude and
          }                      // Flag computed from transfer functions
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkVolumeRayCastSpaceLeapingImageFilter
::ComputeInputExtentsForOutput( int inExt[6], int inDim[3],
                                int outExt[6], vtkImageData *inData )
{
  int inWholeExt[6];
  inData->GetExtent(inWholeExt);

  for ( int i = 0; i < 3; i++ )
    {
    inExt[2*i] = outExt[2*i] * VTK_SL_BLK + inWholeExt[2*i];

    // Extra +1 needed here since we group four cells (which require 5
    // samples) into one element in the min/max tree
    inExt[2*i+1] = (outExt[2*i+1]+1) * VTK_SL_BLK + inWholeExt[2*i] + 1;

    // Clip the extents with the whole extent.
    if (inExt[2*i] < inWholeExt[2*i])
      {
      inExt[2*i] = inWholeExt[2*i];
      }
    if (inExt[2*i+1] > inWholeExt[2*i+1])
      {
      inExt[2*i+1] = inWholeExt[2*i+1];
      }

    inDim[i] = inExt[2*i+1] - inExt[2*i] + 1;
    }
}

//----------------------------------------------------------------------------
// Fill in the min-max space leaping information.
template <class T>
void
vtkVolumeRayCastSpaceLeapingImageFilterMinMaxExecute(
    vtkVolumeRayCastSpaceLeapingImageFilter *self,
    vtkImageData *inData,
    vtkImageData *outData, int outExt[6],
    T )
{

  // the number of independent components for which we need to keep track of
  // min/max
  vtkDataArray * scalars = self->GetCurrentScalars();
  const int components = scalars->GetNumberOfComponents();
  const int independent = self->GetIndependentComponents();
  const int nComponents = (independent) ? components : 1;

  // B. Now fill in the max-min-gradient volume structure

  // B.1 First compute the extents of the input that contribute to this structure

  int inExt[6], inWholeExt[6];
  int inDim[3];
  int outWholeDim[3];
  vtkVolumeRayCastSpaceLeapingImageFilter::ComputeInputExtentsForOutput(
    inExt, inDim, outExt, inData );
  inData->GetExtent(inWholeExt);
  outData->GetDimensions(outWholeDim);

  float shift[4], scale[4];
  self->GetTableShift(shift);
  self->GetTableScale(scale);


  // B.2 Get increments to march through the input extents

  vtkIdType inInc0, inInc1, inInc2;
  inData->GetContinuousIncrements(scalars,
                                  inExt, inInc0, inInc1, inInc2);

  // Get increments to march through the output extents

  const vtkIdType outInc0 = 3*nComponents;
  const vtkIdType outInc1 = outInc0*outWholeDim[0];
  const vtkIdType outInc2 = outInc1*outWholeDim[1];

  // B.3 Now fill in the min-max volume.

  int i, j, k;
  int c;
  int sx1, sx2, sy1, sy2, sz1, sz2;
  int x, y, z;

  T *dptr = static_cast< T * >(scalars->GetVoidPointer(0));
  unsigned short val;
  unsigned short *outBasePtr = static_cast< unsigned short * >(
                                outData->GetScalarPointer());

  // Initialize pointer to the starting extents given by inExt.
  dptr += self->ComputeOffset( inExt, inWholeExt, nComponents );

  // The pointer into the space-leaping output volume.
  unsigned short *tmpPtr, *tmpPtrK, *tmpPtrJ, *tmpPtrI;

  for ( k = 0; k < inDim[2]; k++, dptr += inInc2 )
    {
    sz1 = (k < 1)?(0):((k-1)/4);
    sz2 =             ((k  )/4);
    sz2 = ( k == inDim[2]-1 )?(sz1):(sz2);

    sz1 += outExt[4];
    sz2 += outExt[4];

    // Bounds check
    if (sz2 > outExt[5])
      {
      sz2 = outExt[5];
      }

    tmpPtrK = outBasePtr + sz1 * outInc2;

    for ( j = 0; j < inDim[1]; j++, dptr+= inInc1 )
      {
      sy1 = (j < 1)?(0):((j-1)/4);
      sy2 =             ((j  )/4);
      sy2 = ( j == inDim[1]-1 )?(sy1):(sy2);

      sy1 += outExt[2];
      sy2 += outExt[2];

      // Bounds check
      if (sy2 > outExt[3])
        {
        sy2 = outExt[3];
        }

      tmpPtrJ = tmpPtrK + sy1 * outInc1;

      for ( i = 0; i < inDim[0]; i++ )
        {
        sx1 = (i < 1)?(0):((i-1)/4);
        sx2 =             ((i  )/4);
        sx2 = ( i == inDim[0]-1 )?(sx1):(sx2);

        sx1 += outExt[0];
        sx2 += outExt[0];

        // Bounds check
        if (sx2 > outExt[1])
          {
          sx2 = outExt[1];
          }

        tmpPtrI = tmpPtrJ + sx1 * outInc0;

        for ( c = 0; c < nComponents; c++, tmpPtrI += 3 )
          {
          if ( independent )
            {
            val = static_cast<unsigned short>((*dptr + shift[c]) * scale[c]);
            ++dptr;
            }
          else
            {
            val = static_cast<unsigned short>((*(dptr+components-1) +
                   shift[components-1]) * scale[components-1]);
            dptr += components;
            }

          for ( z = sz1; z <= sz2; z++ )
            {
            for ( y = sy1; y <= sy2; y++ )
              {
              tmpPtr = tmpPtrI + (z-sz1)*outInc2 + (y-sy1)*outInc1;
              for ( x = sx1; x <= sx2; x++, tmpPtr += outInc0 )
                {
                if (val < tmpPtr[0])
                  {
                  tmpPtr[0] = val;
                  }
                if (val > tmpPtr[1])
                  {
                  tmpPtr[1] = val;
                  }
                }
              }
            }
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
// Fill in the maximum gradient magnitude space leaping information.
template <class T>
void
vtkVolumeRayCastSpaceLeapingImageFilterMaxGradientMagnitudeExecute(
    vtkVolumeRayCastSpaceLeapingImageFilter *self,
    vtkImageData *inData,
    vtkImageData *outData, int outExt[6],
    T )
{
  // the number of independent components for which we need to keep track of
  // min/max
  const int nComponents = self->GetNumberOfIndependentComponents();


  // B. Now fill in the max-min-gradient volume structure

  // B.1 First compute the extents of the input that contribute to this structure

  int inExt[6], inWholeExt[6];
  int inDim[3];
  int outWholeDim[3];
  vtkVolumeRayCastSpaceLeapingImageFilter::ComputeInputExtentsForOutput(
    inExt, inDim, outExt, inData );
  inData->GetExtent(inWholeExt);
  outData->GetDimensions(outWholeDim);

  float shift[4], scale[4];
  self->GetTableShift(shift);
  self->GetTableScale(scale);


  // B.2 Get increments to march through the input extents

  vtkIdType inInc0, inInc1, inInc2;
  inData->GetContinuousIncrements(self->GetCurrentScalars(),
                                  inExt, inInc0, inInc1, inInc2);

  // Get increments to march through the output extents

  const vtkIdType outInc0 = 3*nComponents;
  const vtkIdType outInc1 = outInc0*outWholeDim[0];
  const vtkIdType outInc2 = outInc1*outWholeDim[1];

  // B.3 Now fill in the min-max volume.

  int i, j, k;
  int c;
  int sx1, sx2, sy1, sy2, sz1, sz2;
  int x, y, z;

  unsigned char val;
  unsigned short *outBasePtr = static_cast< unsigned short * >(
                                outData->GetScalarPointer());

  // The pointer into the space-leaping output volume.
  unsigned short *tmpPtr, *tmpPtrK, *tmpPtrJ, *tmpPtrI;

  // pointer to the slice of the gradient magnitude
  unsigned char **gsptr = self->GetGradientMagnitude();

  // Initialize pointer to the starting extents given by inExt.
  gsptr += (inExt[4]-inWholeExt[4]);

  for ( k = 0; k < inDim[2]; k++, ++gsptr )
    {
    sz1 = (k < 1)?(0):((k-1)/4);
    sz2 =             ((k  )/4);
    sz2 = ( k == inDim[2]-1 )?(sz1):(sz2);

    sz1 += outExt[4];
    sz2 += outExt[4];

    // Bounds check
    if (sz2 > outExt[5])
      {
      sz2 = outExt[5];
      }

    tmpPtrK = outBasePtr + sz1 * outInc2;

    unsigned char *gptr = *gsptr;

    for ( j = 0; j < inDim[1]; j++, gptr+= inInc1 )
      {
      sy1 = (j < 1)?(0):((j-1)/4);
      sy2 =             ((j  )/4);
      sy2 = ( j == inDim[1]-1 )?(sy1):(sy2);

      sy1 += outExt[2];
      sy2 += outExt[2];

      // Bounds check
      if (sy2 > outExt[3])
        {
        sy2 = outExt[3];
        }

      tmpPtrJ = tmpPtrK + sy1 * outInc1;

      for ( i = 0; i < inDim[0]; i++ )
        {
        sx1 = (i < 1)?(0):((i-1)/4);
        sx2 =             ((i  )/4);
        sx2 = ( i == inDim[0]-1 )?(sx1):(sx2);

        sx1 += outExt[0];
        sx2 += outExt[0];

        // Bounds check
        if (sx2 > outExt[1])
          {
          sx2 = outExt[1];
          }

        tmpPtrI = tmpPtrJ + sx1 * outInc0;

        for ( c = 0; c < nComponents; c++, tmpPtrI += 3 )
          {
          val = *gptr;
          ++gptr;

          for ( z = sz1; z <= sz2; z++ )
            {
            for ( y = sy1; y <= sy2; y++ )
              {
              tmpPtr = tmpPtrI + (z-sz1)*outInc2 + (y-sy1)*outInc1;
              for ( x = sx1; x <= sx2; x++, tmpPtr += outInc0 )
                {

                // Need to keep track of max gradient magnitude in upper
                // eight bits. No need to preserve lower eight (the flag)
                // since we will be recomputing this.
                if (val>(tmpPtr[2]>>8))
                  {
                  tmpPtr[2] = (val<<8);
                  }

                }
              }
            }
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
// Optimized method that does both the following in one pass
// - Fill in the min-max space leaping information.
// - Fill in the maximum gradient magnitude space leaping information.
template <class T>
void
vtkVolumeRayCastSpaceLeapingImageFilterMinMaxAndMaxGradientMagnitudeExecute(
    vtkVolumeRayCastSpaceLeapingImageFilter *self,
    vtkImageData *inData,
    vtkImageData *outData, int outExt[6],
    T )
{
  // the number of independent components for which we need to keep track of
  // min/max
  vtkDataArray * scalars = self->GetCurrentScalars();
  const int components = scalars->GetNumberOfComponents();
  const int independent = self->GetIndependentComponents();
  const int nComponents = (independent) ? components : 1;


  // B.1 First compute the extents of the input that contribute to this structure

  int inExt[6], inWholeExt[6];
  int inDim[3];
  int outWholeDim[3];
  vtkVolumeRayCastSpaceLeapingImageFilter::ComputeInputExtentsForOutput(
    inExt, inDim, outExt, inData );
  inData->GetExtent(inWholeExt);
  outData->GetDimensions(outWholeDim);

  float shift[4], scale[4];
  self->GetTableShift(shift);
  self->GetTableScale(scale);


  // B.2 Get increments to march through the input extents

  vtkIdType inInc0, inInc1, inInc2;
  inData->GetContinuousIncrements(scalars,
                                  inExt, inInc0, inInc1, inInc2);

  // Get increments to march through the output extents

  const vtkIdType outInc0 = 3*nComponents;
  const vtkIdType outInc1 = outInc0*outWholeDim[0];
  const vtkIdType outInc2 = outInc1*outWholeDim[1];

  // B.3 Now fill in the min-max and gradient max structure

  int i, j, k;
  int c;
  int sx1, sx2, sy1, sy2, sz1, sz2;
  int x, y, z;


  T *dptr = static_cast< T * >(scalars->GetVoidPointer(0));
  unsigned char val;
  unsigned short minMaxVal;
  unsigned short *outBasePtr = static_cast< unsigned short * >(
                                outData->GetScalarPointer());

  // pointer to the slice of the gradient magnitude
  unsigned char **gsptr = self->GetGradientMagnitude();

  // Initialize pointers to the starting extents given by inExt.
  gsptr += (inExt[4]-inWholeExt[4]);  // pointer to slice gradient
  dptr += self->ComputeOffset( inExt, inWholeExt, nComponents );

  // The pointer into the space-leaping output volume.
  unsigned short *tmpPtr, *tmpPtrK, *tmpPtrJ, *tmpPtrI;

  for ( k = 0; k < inDim[2]; k++, dptr += inInc2, ++gsptr )
    {
    sz1 = (k < 1)?(0):((k-1)/4);
    sz2 =             ((k  )/4);
    sz2 = ( k == inDim[2]-1 )?(sz1):(sz2);

    sz1 += outExt[4];
    sz2 += outExt[4];

    // Bounds check
    if (sz2 > outExt[5])
      {
      sz2 = outExt[5];
      }

    tmpPtrK = outBasePtr + sz1 * outInc2;

    unsigned char *gptr = *gsptr;

    for ( j = 0; j < inDim[1]; j++, dptr+= inInc1, gptr+= inInc1 )
      {
      sy1 = (j < 1)?(0):((j-1)/4);
      sy2 =             ((j  )/4);
      sy2 = ( j == inDim[1]-1 )?(sy1):(sy2);

      sy1 += outExt[2];
      sy2 += outExt[2];

      // Bounds check
      if (sy2 > outExt[3])
        {
        sy2 = outExt[3];
        }

      tmpPtrJ = tmpPtrK + sy1 * outInc1;

      for ( i = 0; i < inDim[0]; i++ )
        {
        sx1 = (i < 1)?(0):((i-1)/4);
        sx2 =             ((i  )/4);
        sx2 = ( i == inDim[0]-1 )?(sx1):(sx2);

        sx1 += outExt[0];
        sx2 += outExt[0];

        // Bounds check
        if (sx2 > outExt[1])
          {
          sx2 = outExt[1];
          }

        tmpPtrI = tmpPtrJ + sx1 * outInc0;

        for ( c = 0; c < nComponents; c++, tmpPtrI += 3 )
          {
          val = *gptr;
          ++gptr;

          if ( independent )
            {
            minMaxVal = static_cast<unsigned short>((*dptr + shift[c]) * scale[c]);
            ++dptr;
            }
          else
            {
            minMaxVal = static_cast<unsigned short>((*(dptr+components-1) +
                   shift[components-1]) * scale[components-1]);
            dptr += components;
            }

          for ( z = sz1; z <= sz2; z++ )
            {
            for ( y = sy1; y <= sy2; y++ )
              {

              tmpPtr = tmpPtrI + (z-sz1)*outInc2 + (y-sy1)*outInc1;
              for ( x = sx1; x <= sx2; x++, tmpPtr += outInc0 )
                {

                if (minMaxVal<tmpPtr[0])
                  {
                  tmpPtr[0] = minMaxVal;
                  }
                if (minMaxVal>tmpPtr[1])
                  {
                  tmpPtr[1] = minMaxVal;
                  }
                if (val>(tmpPtr[2]>>8))
                  {
                  tmpPtr[2] = (val<<8);
                  }
                }
              }
            }
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkVolumeRayCastSpaceLeapingImageFilter
::FillScalarAndGradientOpacityFlags( vtkImageData *outData, int outExt[6] )
{
  // Get increments to march through the output

  vtkIdType outInc0, outInc1, outInc2;
  outData->GetContinuousIncrements(this->CurrentScalars,
                                   outExt, outInc0, outInc1, outInc2);

  // Now process the flags

  unsigned short *tmpPtr = static_cast< unsigned short * >(
                outData->GetScalarPointerForExtent(outExt));
  unsigned short *minNonZeroScalarIndex
                     = this->GetMinNonZeroScalarIndex();
  unsigned char  *minNonZeroGradientMagnitudeIndex
                     = this->GetMinNonZeroGradientMagnitudeIndex();

  int i, j, k, c, loop;

  // the number of independent components for which we need to keep track of
  // min/max/gradient
  const int nComponents = this->GetNumberOfIndependentComponents();

  // Loop over the data with in the supplied extents

  for (k = outExt[4]; k <= outExt[5]; ++k, tmpPtr += outInc2)
    {
    for (j = outExt[2]; j <= outExt[3]; ++j, tmpPtr += outInc1)
      {
      for (i = outExt[0]; i <= outExt[1]; ++i)
        {
        for ( c = 0; c < nComponents; ++c, tmpPtr += 3 )
          {

          // We definite have 0 opacity because our maximum scalar value in
          // this region is below the minimum scalar value with non-zero opacity
          // for this component
          if ( tmpPtr[1] < minNonZeroScalarIndex[c] )
            {
            tmpPtr[2] &= 0xff00;
            }
          // We have 0 opacity because we are using gradient magnitudes and
          // the maximum gradient magnitude in this area is below the minimum
          // gradient magnitude with non-zero opacity for this component
          else if ( (tmpPtr[2]>>8) < minNonZeroGradientMagnitudeIndex[c] )
            {
            tmpPtr[2] &= 0xff00;
            }
          // We definitely have non-zero opacity because our minimum scalar
          // value is lower than our first scalar with non-zero opacity, and
          // the maximum scalar value is greater than this threshold - so
          // we must encounter scalars with opacity in between
          else if ( tmpPtr[0] < minNonZeroScalarIndex[c] )
            {
            tmpPtr[2] &= 0xff00;
            tmpPtr[2] |= 0x0001;
            }
          // We have to search between min scalar value and the
          // max scalar stored in the minmax volume to look for non-zero
          // opacity since both values must be above our first non-zero
          // threshold so we don't have information in this area
          else
            {
            for ( loop = tmpPtr[0]; loop <= tmpPtr[1]; ++loop )
              {
              if ( this->ScalarOpacityTable[c][loop] )
                {
                break;
                }
              }
            if ( loop <= tmpPtr[1] )
              {
              tmpPtr[2] &= 0xff00;
              tmpPtr[2] |= 0x0001;
              }
            else
              {
              tmpPtr[2] &= 0xff00;
              }
            }
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkVolumeRayCastSpaceLeapingImageFilter
::FillScalarOpacityFlags( vtkImageData *outData, int outExt[6] )
{
  // Get increments to march through the output

  vtkIdType outInc0, outInc1, outInc2;
  outData->GetContinuousIncrements(this->CurrentScalars,
                                   outExt, outInc0, outInc1, outInc2);

  // Now process the flags

  unsigned short *tmpPtr = static_cast< unsigned short * >(
                outData->GetScalarPointerForExtent(outExt));
  unsigned short *minNonZeroScalarIndex
                     = this->GetMinNonZeroScalarIndex();

  int i, j, k, c, loop;

  // the number of independent components for which we need to keep track of
  // min/max/gradient
  const int nComponents = this->GetNumberOfIndependentComponents();

  // Loop over the data with in the supplied extents

  for (k = outExt[4]; k <= outExt[5]; ++k, tmpPtr += outInc2)
    {
    for (j = outExt[2]; j <= outExt[3]; ++j, tmpPtr += outInc1)
      {
      for (i = outExt[0]; i <= outExt[1]; ++i)
        {
        for ( c = 0; c < nComponents; ++c, tmpPtr += 3 )
          {

          // We definite have 0 opacity because our maximum scalar value in
          // this region is below the minimum scalar value with non-zero opacity
          // for this component
          if ( tmpPtr[1] < minNonZeroScalarIndex[c] )
            {
            tmpPtr[2] &= 0xff00;
            }
          // We definitely have non-zero opacity because our minimum scalar
          // value is lower than our first scalar with non-zero opacity, and
          // the maximum scalar value is greater than this threshold - so
          // we must encounter scalars with opacity in between
          else if ( tmpPtr[0] < minNonZeroScalarIndex[c] )
            {
            tmpPtr[2] &= 0xff00;
            tmpPtr[2] |= 0x0001;
            }
          // We have to search between min scalar value and the
          // max scalar stored in the minmax volume to look for non-zero
          // opacity since both values must be above our first non-zero
          // threshold so we don't have information in this area
          else
            {
            for ( loop = tmpPtr[0]; loop <= tmpPtr[1]; ++loop )
              {
              if ( this->ScalarOpacityTable[c][loop] )
                {
                break;
                }
              }
            if ( loop <= tmpPtr[1] )
              {
              tmpPtr[2] &= 0xff00;
              tmpPtr[2] |= 0x0001;
              }
            else
              {
              tmpPtr[2] &= 0xff00;
              }
            }
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkVolumeRayCastSpaceLeapingImageFilter::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int vtkNotUsed(id))
{
#ifdef vtkVolumeRayCastSpaceLeapingImageFilter_DEBUG
  std::cout << "Thread id = " << id << std::endl;
#endif

  // A. Initialize the data with a blank flag.

  // - Get the number of independent components for which we need to keep
  //   track of min/max
  if (!this->GetCurrentScalars())
    {
    return;
    }

  const int components = this->GetCurrentScalars()->GetNumberOfComponents();
  const int nComponents = (this->GetIndependentComponents()) ? components : 1;

  // Clear the output if we are computing the min-max. In other cases, we
  // will be re-using the cache. (See the method AllocateOutputData)

  if (this->ComputeMinMax)
    {
    vtkVolumeRayCastSpaceLeapingImageFilterClearOutput(this->CurrentScalars,
                                                       outData[0], outExt, nComponents );
    }


  // If only scalar min-max need to be re-computed

  if (this->ComputeMinMax && !this->ComputeGradientOpacity)
    {
    int scalarType   = this->CurrentScalars->GetDataType();
    switch (scalarType)
      {
      vtkTemplateMacro(
        vtkVolumeRayCastSpaceLeapingImageFilterMinMaxExecute(
          this, inData[0][0], outData[0], outExt, static_cast<VTK_TT>(0))
        );
      default:
        vtkErrorMacro("Unknown scalar type");
        return;
      }
    }

  // If only gradient max needs to be recomputed

  else if (this->ComputeGradientOpacity && !this->ComputeMinMax)
    {
    int scalarType   = this->CurrentScalars->GetDataType();
    switch (scalarType)
      {
      vtkTemplateMacro(
        vtkVolumeRayCastSpaceLeapingImageFilterMaxGradientMagnitudeExecute(
          this, inData[0][0], outData[0], outExt, static_cast<VTK_TT>(0))
        );
      default:
        vtkErrorMacro("Unknown scalar type");
        return;
      }
    }

  // If both scalar min-max need to be re-computed and gradient max needs to
  // be re-computed

  else if (this->ComputeGradientOpacity && this->ComputeMinMax)
    {
    int scalarType   = this->CurrentScalars->GetDataType();
    switch (scalarType)
      {
      vtkTemplateMacro(
        vtkVolumeRayCastSpaceLeapingImageFilterMinMaxAndMaxGradientMagnitudeExecute(
          this, inData[0][0], outData[0], outExt, static_cast<VTK_TT>(0))
        );
      default:
        vtkErrorMacro("Unknown scalar type");
        return;
      }
    }


  // Update the flags now for this extent. There are two specialized methods
  // here, depending on what mode we are in, so that we may do the flag update
  // in one pass through the data.

  if (this->UpdateGradientOpacityFlags)
    {
    // Process the flags based on the computed min-max volume
    this->FillScalarAndGradientOpacityFlags(outData[0], outExt);
    }
  else
    {
    this->FillScalarOpacityFlags(outData[0], outExt);
    }
}

//----------------------------------------------------------------------------
// Override superclass method to maintain a last successful execution time
int vtkVolumeRayCastSpaceLeapingImageFilter::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
#ifdef vtkVolumeRayCastSpaceLeapingImageFilter_DEBUG
  cout << "ComputingGradientOpacity: " << ComputeGradientOpacity;
  cout << " ComputingMinMax: " << ComputeMinMax << " UpdatingFlags: 1" << endl;
#endif

  // Find the first non-zero scalar opacity and gradient opacity points on
  // the respective transfer functions

  this->ComputeFirstNonZeroOpacityIndices();

  // The actual work is done in the line below.
  if (this->Superclass::RequestData(request, inputVector, outputVector))
    {

    // if we recomputed the first two shorts in the output, update this.
    if (this->ComputeGradientOpacity || this->ComputeMinMax)
      {
      this->LastMinMaxBuildTime.Modified();
      }

    // flags were rebuilt. update this.
    this->LastMinMaxFlagTime.Modified();

    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkVolumeRayCastSpaceLeapingImageFilter::RequestInformation (
  vtkInformation       * request,
  vtkInformationVector** inputVector,
  vtkInformationVector * outputVector)
{
  this->vtkImageAlgorithm::RequestInformation(request, inputVector, outputVector);

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Output scalar type is unsigned short,
  // 3 unsigned short values are needed to represent the min, max and gradient,
  // flag values. This is to be done for each independent component.

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo,
      VTK_UNSIGNED_SHORT,
      3 * this->GetNumberOfIndependentComponents() );

  // The whole extent of the output is the whole extent of the input divided
  // by the block size along each dimension

  int outWholeExt[6], inWholeExtent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inWholeExtent);
  this->InternalRequestUpdateExtent(outWholeExt, inWholeExtent);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), outWholeExt, 6);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outWholeExt, 6);

  return 1;
}

//----------------------------------------------------------------------------
int vtkVolumeRayCastSpaceLeapingImageFilter
::GetNumberOfIndependentComponents()
{
  // the number of independent components for which we need to keep track of
  // min/max
  if(this->CurrentScalars)
    {
    const int components = this->CurrentScalars->GetNumberOfComponents();
    return ((this->IndependentComponents) ? components : 1);
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkVolumeRayCastSpaceLeapingImageFilter
::ComputeFirstNonZeroOpacityIndices()
{
  // Find the first non-zero scalar opacity and gradient opacity points on
  // the respective transfer functions

  const int nComponents = this->GetNumberOfIndependentComponents();

  // Initialize these arrays.
  if (this->MinNonZeroScalarIndex)
    {
    delete [] this->MinNonZeroScalarIndex;
    this->MinNonZeroScalarIndex = NULL;
    }
  if (this->MinNonZeroGradientMagnitudeIndex)
    {
    delete [] this->MinNonZeroGradientMagnitudeIndex;
    this->MinNonZeroGradientMagnitudeIndex = NULL;
    }

  // Update the flags now
  int i;
  this->MinNonZeroScalarIndex = new unsigned short [nComponents];
  for ( int c = 0; c < nComponents; c++ )
    {
    for ( i = 0; i < this->TableSize[c]; i++ )
      {
      if ( this->ScalarOpacityTable[c][i] )
        {
        break;
        }
      }
    this->MinNonZeroScalarIndex[c] = i;
    }

  this->MinNonZeroGradientMagnitudeIndex = new unsigned char [nComponents];
  for ( int c = 0; c < nComponents; c++ )
    {
    for ( i = 0; i < 256; i++ )
      {
      if ( this->GradientOpacityTable[c][i] )
        {
        break;
        }
      }
    this->MinNonZeroGradientMagnitudeIndex[c] = i;
    }
}

//----------------------------------------------------------------------------
unsigned short * vtkVolumeRayCastSpaceLeapingImageFilter
::GetMinNonZeroScalarIndex()
{
  return this->MinNonZeroScalarIndex;
}

//----------------------------------------------------------------------------
unsigned char * vtkVolumeRayCastSpaceLeapingImageFilter
::GetMinNonZeroGradientMagnitudeIndex()
{
  return this->MinNonZeroGradientMagnitudeIndex;
}

//----------------------------------------------------------------------------
void vtkVolumeRayCastSpaceLeapingImageFilter
::SetGradientMagnitude( unsigned char ** gradientMagnitude )
{
  this->GradientMagnitude = gradientMagnitude;
}

//----------------------------------------------------------------------------
unsigned char ** vtkVolumeRayCastSpaceLeapingImageFilter
::GetGradientMagnitude()
{
  return this->GradientMagnitude;
}

//----------------------------------------------------------------------------
void vtkVolumeRayCastSpaceLeapingImageFilter
::SetScalarOpacityTable( int c, unsigned short * t )
{
  this->ScalarOpacityTable[c] = t;
}

//----------------------------------------------------------------------------
void vtkVolumeRayCastSpaceLeapingImageFilter
::SetGradientOpacityTable( int c, unsigned short * t )
{
  this->GradientOpacityTable[c] = t;
}

//----------------------------------------------------------------------------
unsigned short * vtkVolumeRayCastSpaceLeapingImageFilter
::GetMinMaxVolume( int size[4] )
{
  if (vtkImageData *output = this->GetOutput())
    {
    int dims[3];
    output->GetDimensions(dims);
    size[0] = dims[0];
    size[1] = dims[1];
    size[2] = dims[2];
    size[3] = this->GetNumberOfIndependentComponents();

    return static_cast< unsigned short * >(output->GetScalarPointer());
    }
  return NULL;
}

//----------------------------------------------------------------------------
// Fill in the min-max space leaping information.
vtkIdType vtkVolumeRayCastSpaceLeapingImageFilter
::ComputeOffset( const int ext[6], const int wholeExt[6], int nComponents )
{
  int wDim[3] = { wholeExt[1]-wholeExt[0]+1,
                  wholeExt[3]-wholeExt[2]+1,
                  wholeExt[5]-wholeExt[4]+1 };

  // computation is done in parts to avoid int overflow
  vtkIdType offset = ext[4]-wholeExt[4];
  offset *= wDim[1];
  offset += ext[2]-wholeExt[2];
  offset *= wDim[0];
  offset += ext[0]-wholeExt[0];
  offset *= nComponents;

  return offset;
}

//----------------------------------------------------------------------------
// Allocate the output data, caching if necessary. Caching may result in
// invalid outputs and should be turned on, only when this filter is used
// as an internal ivar of the vtkFixedPointVolumeRayCastMapper.
void vtkVolumeRayCastSpaceLeapingImageFilter
::AllocateOutputData(vtkImageData *output, int *uExtent)
{ 
  // set the extent to be the update extent
  output->SetExtent(uExtent);

  if (this->Cache)
    {

    int extent[6];
    this->Cache->GetExtent(extent);
    if (extent[0] == uExtent[0] && extent[1] == uExtent[1] &&
        extent[2] == uExtent[2] && extent[3] == uExtent[3] &&
        extent[4] == uExtent[4] && extent[5] == uExtent[5] &&
        this->Cache->GetNumberOfScalarComponents() ==
                    output->GetNumberOfScalarComponents())
      {
      // Reuse the cache since it has the same dimensions. We may not be
      // updating all flags

      // This is absolutely scary code, if used as a standard VTK imaging filter,
      // but since the filter will be used only as an iVar of
      // vtkFixedPointVolumeRayCastMapper, we need a caching mechanism to avoid
      // reallocation of memory and re-update of certain bits in the Min-max
      // structure. In the interest of speed, we resort to a wee bit of ugly
      // code.
      output->GetPointData()->SetScalars(
          this->Cache->GetPointData()->GetScalars() );
      return;
      }
    }

  // Otherwise allocate output afresh
  output->AllocateScalars();
}

//----------------------------------------------------------------------------
vtkImageData *vtkVolumeRayCastSpaceLeapingImageFilter
::AllocateOutputData(vtkDataObject *output)
{ 
  // Call the superclass method
  return vtkImageAlgorithm::AllocateOutputData(output);
}

//----------------------------------------------------------------------------
#ifdef vtkVolumeRayCastSpaceLeapingImageFilter_DEBUG
void vtkVolumeRayCastSpaceLeapingImageFilter
::WriteMinMaxVolume(
  int component, unsigned short *minMaxVolume,
  int minMaxVolumeSize[4],
  const char *filename )
{
  vtkImageData *image = vtkImageData::New();
  image->SetExtent(0, minMaxVolumeSize[0]-1,
                   0, minMaxVolumeSize[1]-1,
                   0, minMaxVolumeSize[2]-1);
  image->SetScalarTypeToUnsignedShort();
  image->AllocateScalars();

  const int nComponents = minMaxVolumeSize[3];
  const int inc = nComponents * 3;
  unsigned short *pSrc = minMaxVolume + component;
  unsigned short *pDst = static_cast< unsigned short * >(
                                image->GetScalarPointer());
  // Do computation in parts to avoid int overfloat
  vtkIdType nVoxels = minMaxVolumeSize[0];
  nVoxels *= minMaxVolumeSize[1]
  nVoxels *= minMaxVolumeSize[2];

  for (vtkIdType i = 0; i < nVoxels; ++i, pSrc += inc, ++pDst)
    {
    *pDst = *pSrc;
    }

  vtkMetaImageWriter *writer = vtkMetaImageWriter::New();
  writer->SetFileName(filename);
  writer->SetInput(image);
  writer->Write();

  writer->Delete();
  image->Delete();
}

#endif
