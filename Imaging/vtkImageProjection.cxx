/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageProjection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageProjection.h"

#include "vtkImageData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkObjectFactory.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkMath.h"

#include <math.h>

vtkStandardNewMacro(vtkImageProjection);

//----------------------------------------------------------------------------
vtkImageProjection::vtkImageProjection()
{
  this->Operation = VTK_PROJECTION_AVERAGE;
  this->SliceDirection = 2;
  this->SliceRange[0] = VTK_INT_MIN;
  this->SliceRange[1] = VTK_INT_MAX;
  this->OutputScalarType = 0;
  this->MultiSliceOutput = 0;
}

//----------------------------------------------------------------------------
vtkImageProjection::~vtkImageProjection()
{
}

//----------------------------------------------------------------------------
int vtkImageProjection::RequestInformation(
  vtkInformation *, vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int extent[6];
  int range[2];
  double origin[3];
  double spacing[3];
  double sliceSpacing;
  int dimIndex;
  int scalarType;

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  inInfo->Get(vtkDataObject::SPACING(), spacing);
  inInfo->Get(vtkDataObject::ORIGIN(), origin);

  // get the direction along which to sum slices
  dimIndex = this->GetSliceDirection();

  // clamp the range to the whole extent
  this->GetSliceRange(range);
  if (range[0] < extent[2*dimIndex])
    {
    range[0] = extent[2*dimIndex];
    }
  if (range[1] > extent[2*dimIndex+1])
    {
    range[1] = extent[2*dimIndex+1];
    }

  // set new origin to be in the center of the stack of slices
  sliceSpacing = spacing[dimIndex];
  origin[dimIndex] = (origin[dimIndex] +
                      0.5*sliceSpacing*(range[0] + range[1]));

  if (this->GetMultiSliceOutput())
    {
    // output extent is input extent, decreased by the slice range
    extent[2*dimIndex] -= range[0];
    extent[2*dimIndex+1] -= range[1];
    }
  else
    {
    // set new extent to single-slice
    extent[2*dimIndex] = 0;
    extent[2*dimIndex+1] = 0;
    }

  // set the output scalar type
  scalarType = this->GetOutputScalarType();

  // set the output information
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    extent, 6);
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);

  // if requested, change the type to float or double
  if (scalarType == VTK_FLOAT || scalarType == VTK_DOUBLE)
    {
    vtkDataObject::SetPointDataActiveScalarInfo(outInfo, scalarType, -1);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageProjection::RequestUpdateExtent(
  vtkInformation *, vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int outExt[6];
  int inExt[6];
  int extent[6];
  int range[2];
  int dimIndex;

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExt);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);

  // initialize input extent to output extent
  inExt[0] = outExt[0];
  inExt[1] = outExt[1];
  inExt[2] = outExt[2];
  inExt[3] = outExt[3];
  inExt[4] = outExt[4];
  inExt[5] = outExt[5];

  // get the direction along which to sum slices
  dimIndex = this->GetSliceDirection();

  // clamp the range to the whole extent
  this->GetSliceRange(range);
  if (range[0] < extent[2*dimIndex])
    {
    range[0] = extent[2*dimIndex];
    }
  if (range[1] > extent[2*dimIndex+1])
    {
    range[1] = extent[2*dimIndex+1];
    }

  // input range is the output range plus the specified slice range
  inExt[2*dimIndex] += range[0];
  inExt[2*dimIndex+1] += range[1];

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  return 1;
}

//----------------------------------------------------------------------------
// rounding functions for each type

inline void vtkProjectionRound(double val, signed char& rnd)
{
  rnd = static_cast<signed char>(vtkMath::Floor(val + 0.5));
}

inline void vtkProjectionRound(double val, char& rnd)
{
  rnd = static_cast<char>(vtkMath::Floor(val + 0.5));
}

inline void vtkProjectionRound(double val, unsigned char& rnd)
{
  rnd = static_cast<unsigned char>(vtkMath::Floor(val + 0.5));
}

inline void vtkProjectionRound(double val, short& rnd)
{
  rnd = static_cast<short>(vtkMath::Floor(val + 0.5));
}

inline void vtkProjectionRound(double val, unsigned short& rnd)
{
  rnd = static_cast<unsigned short>(vtkMath::Floor(val + 0.5));
}

inline void vtkProjectionRound(double val, int& rnd)
{
  rnd = static_cast<int>(vtkMath::Floor(val + 0.5));
}

inline void vtkProjectionRound(double val, unsigned int& rnd)
{
  rnd = static_cast<unsigned int>(floor(val + 0.5));
}

inline void vtkProjectionRound(double val, long& rnd)
{
  rnd = static_cast<unsigned int>(floor(val + 0.5));
}

inline void vtkProjectionRound(double val, unsigned long& rnd)
{
  rnd = static_cast<unsigned int>(floor(val + 0.5));
}

#ifdef VTK_LONG_LONG_MIN
inline void vtkProjectionRound(double val, long long& rnd)
{
  rnd = static_cast<long long>(floor(val + 0.5));
}
#else /* VTK_LONG_LONG_MIN */
#ifdef VTK___INT64_MIN
inline void vtkProjectionRound(double val, __int64& rnd)
{
  rnd = static_cast<__int64>(floor(val + 0.5));
}
#endif /* VTK___INT64_MIN */
#endif /* VTK_LONG_LONG_MIN */

#ifdef VTK_UNSIGNED_LONG_LONG_MIN
inline void vtkProjectionRound(double val, unsigned long long& rnd)
{
  rnd = static_cast<unsigned long long>(floor(val + 0.5));
}
#else /* VTK_UNSIGNED_LONG_LONG_MIN */
#ifdef VTK_UNSIGNED__INT64_MIN
inline void vtkProjectionRound(double val, unsigned __int64& clamp)
{
  rnd = static_cast<unsigned __int64>(floor(rnd + 0.5));
}
#endif /* VTK_UNSIGNED__INT64_MIN */
#endif /* VTK_UNSIGNED_LONG_LONG_MIN */

inline void vtkProjectionRound(double val, float& rnd)
{
  rnd = static_cast<float>(val);
}

inline void vtkProjectionRound(double val, double& rnd)
{
  rnd = static_cast<double>(val);
}

//----------------------------------------------------------------------------
// clamping functions for each type

inline void vtkProjectionClamp(double val, signed char& clamp)
{
  if (val >= VTK_SIGNED_CHAR_MIN)
    {
    if (val <= VTK_SIGNED_CHAR_MAX)
      {
      vtkProjectionRound(val, clamp);
      return;
      }
    clamp = VTK_SIGNED_CHAR_MAX;
    return;
    }
  clamp = VTK_SIGNED_CHAR_MIN;
  return;
}

inline void vtkProjectionClamp(double val, char& clamp)
{
  if (val >= VTK_CHAR_MIN)
    {
    if (val <= VTK_CHAR_MAX)
      {
      vtkProjectionRound(val, clamp);
      return;
      }
    clamp = VTK_CHAR_MAX;
    return;
    }
  clamp = VTK_CHAR_MIN;
  return;
}

inline void vtkProjectionClamp(double val, unsigned char& clamp)
{
  if (val >= VTK_UNSIGNED_CHAR_MIN)
    {
    if (val <= VTK_UNSIGNED_CHAR_MAX)
      {
      vtkProjectionRound(val, clamp);
      return;
      }
    clamp = VTK_UNSIGNED_CHAR_MAX;
    return;
    }
  clamp = VTK_UNSIGNED_CHAR_MIN;
  return;
}

inline void vtkProjectionClamp(double val, short& clamp)
{
  if (val >= VTK_SHORT_MIN)
    {
    if (val <= VTK_SHORT_MAX)
      {
      vtkProjectionRound(val, clamp);
      return;
      }
    clamp = VTK_SHORT_MAX;
    return;
    }
  clamp = VTK_SHORT_MIN;
  return;
}

inline void vtkProjectionClamp(double val, unsigned short& clamp)
{
  if (val >= VTK_UNSIGNED_SHORT_MIN)
    {
    if (val <= VTK_UNSIGNED_SHORT_MAX)
      {
      vtkProjectionRound(val, clamp);
      return;
      }
    clamp = VTK_UNSIGNED_SHORT_MAX;
    return;
    }
  clamp = VTK_UNSIGNED_SHORT_MIN;
  return;
}

inline void vtkProjectionClamp(double val, int& clamp)
{
  if (val >= VTK_INT_MIN)
    {
    if (val <= VTK_INT_MAX)
      {
      vtkProjectionRound(val, clamp);
      return;
      }
    clamp = VTK_INT_MAX;
    return;
    }
  clamp = VTK_INT_MIN;
  return;
}

inline void vtkProjectionClamp(double val, unsigned int& clamp)
{
  if (val >= VTK_UNSIGNED_INT_MIN)
    {
    if (val <= VTK_UNSIGNED_INT_MAX)
      {
      vtkProjectionRound(val, clamp);
      return;
      }
    clamp = VTK_UNSIGNED_INT_MAX;
    return;
    }
  clamp = VTK_UNSIGNED_INT_MIN;
  return;
}

inline void vtkProjectionClamp(double val, long& clamp)
{
  if (val >= VTK_LONG_MIN)
    {
    if (val <= VTK_LONG_MAX)
      {
      vtkProjectionRound(val, clamp);
      return;
      }
    clamp = VTK_LONG_MAX;
    return;
    }
  clamp = VTK_LONG_MIN;
  return;
}

inline void vtkProjectionClamp(double val, unsigned long& clamp)
{
  if (val >= VTK_UNSIGNED_LONG_MIN)
    {
    if (val <= VTK_UNSIGNED_LONG_MAX)
      {
      vtkProjectionRound(val, clamp);
      return;
      }
    clamp = VTK_UNSIGNED_LONG_MAX;
    return;
    }
  clamp = VTK_UNSIGNED_LONG_MIN;
  return;
}

#if defined(VTK_LONG_LONG_MIN) && defined(VTK_LONG_LONG_MAX)
inline void vtkProjectionClamp(double val, long long& clamp)
{
  if (val >= VTK_LONG_LONG_MIN)
    {
    if (val <= VTK_LONG_LONG_MAX)
      {
      vtkProjectionRound(val, clamp);
      return;
      }
    clamp = VTK_LONG_LONG_MAX;
    return;
    }
  clamp = VTK_LONG_LONG_MIN;
  return;
}
#endif /* defined(VTK_LONG_LONG_MIN) && defined(VTK_LONG_LONG_MAX) */

#if defined(VTK_UNSIGNED_LONG_LONG_MIN) && defined(VTK_UNSIGNED_LONG_LONG_MAX)
inline void vtkProjectionClamp(double val, unsigned long long& clamp)
{
  if (val >= VTK_UNSIGNED_LONG_LONG_MIN)
    {
    if (val <= VTK_UNSIGNED_LONG_LONG_MAX)
      {
      vtkProjectionRound(val, clamp);
      return;
      }
    clamp = VTK_UNSIGNED_LONG_LONG_MAX;
    return;
    }
  clamp = VTK_UNSIGNED_LONG_LONG_MIN;
  return;
}
#endif
/* defined(VTK_UNSIGNED_LONG_LONG_MIN) && defined(VTK_UNSIGNED_LONG_LONG_MAX)*/

#if defined(VTK_TYPE_USE___INT64) /* to ensure that __int64 is unique */
#if defined(VTK___INT64_MIN) && defined(VTK___INT64_MAX)
inline void vtkProjectionClamp(double val, __int64& clamp)
{
  if (val >= VTK___INT64_MIN)
    {
    if (val <= VTK___INT64_MAX)
      {
      vtkProjectionRound(val, clamp);
      return;
      }
    clamp = VTK___INT64_MAX;
    return;
    }
  clamp = VTK___INT64_MIN;
  return;
}
#endif /* defined(VTK___INT64_MIN) && defined(VTK___INT64_MAX) */
#endif /* defined(VTK_TYPE_USE___INT64) */

#if defined(VTK_TYPE_USE___INT64) && defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
#if defined(VTK_UNSIGNED__INT64_MIN) && defined(VTK_UNSIGNED__INT64_MAX)
inline void vtkProjectionClamp(double val, unsigned __int64& clamp)
{
  if (val >= VTK_UNSIGNED__INT64_MIN)
    {
    if (val <= VTK_UNSIGNED__INT64_MAX)
      {
      vtkProjectionRound(val, clamp);
      return;
      }
    clamp = VTK_UNSIGNED__INT64_MAX;
    return;
    }
  clamp = VTK_UNSIGNED__INT64_MIN;
  return;
}
#endif
/* defined(VTK_UNSIGNED__INT64_MIN) && defined(VTK_UNSIGNED__INT64_MAX) */
#endif
/* defined(VTK_TYPE_USE___INT64) && defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE) */

inline void vtkProjectionClamp(double val, float& clamp)
{
  clamp = val;
}

inline void vtkProjectionClamp(double val, double& clamp)
{
  clamp = val;
}

//----------------------------------------------------------------------------
template <class T1, class T2>
void vtkImageProjectionExecute(vtkImageProjection *self,
                               vtkImageData *inData, T1 *inPtr,
                               vtkImageData *outData, T2 *outPtr,
                               int outExt[6], int id)
{
  vtkIdType outIncX, outIncY, outIncZ;
  vtkIdType inInc[3];
  int inExt[6];

  // get increments to march through data
  inData->GetExtent(inExt);
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  int numscalars = inData->GetNumberOfScalarComponents();
  int rowlen = (outExt[1] - outExt[0] + 1)*numscalars;

  // get the operation
  int operation = self->GetOperation();

  // get the dimension along which to do the projection
  int dimIndex = self->GetSliceDirection();
  if (dimIndex < 0)
    {
    dimIndex = 0;
    }
  else if (dimIndex > 2)
    {
    dimIndex = 2;
    }

  // clamp the range to the whole extent
  int range[2];
  self->GetSliceRange(range);
  if (range[0] < inExt[2*dimIndex])
    {
    range[0] = inExt[2*dimIndex];
    }
  if (range[1] > inExt[2*dimIndex+1])
    {
    range[1] = inExt[2*dimIndex+1];
    }
  int numSlices = range[1] - range[0] + 1;

  // averaging requires double precision summation
  double *rowBuffer = 0;
  if (operation == VTK_PROJECTION_AVERAGE ||
      operation == VTK_PROJECTION_SUM)
    {
    rowBuffer = new double[rowlen];
    }

  unsigned long count = 0;
  unsigned long target = ((unsigned long)(outExt[3]-outExt[2]+1)
                          *(outExt[5]-outExt[4]+1));
  target++;

  // Loop through output pixels
  for (int idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    T1 *inPtrY = inPtr;
    for (int idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      if (!id)
        {
        if (!(count%target))
          {
          self->UpdateProgress(count/(1.0*target));
          }
        count++;
        }

      // ====== code for handling average and sum ======
      if (operation == VTK_PROJECTION_AVERAGE ||
          operation == VTK_PROJECTION_SUM)
        {
        T1 *inSlicePtr = inPtrY;
        double *rowPtr = rowBuffer;

        // initialize using first row
        T1 *inPtrX = inSlicePtr;
        for (int j = 0; j < rowlen; j++)
          {
          *rowPtr++ = *inPtrX++;
          }
        inSlicePtr += inInc[dimIndex];

        // perform the summation
        for (int sliceIdx = 1; sliceIdx < numSlices; sliceIdx++)
          {
          inPtrX = inSlicePtr;
          rowPtr = rowBuffer;

          for (int i = 0; i < rowlen; i++)
            {
            *rowPtr++ += *inPtrX++;
            }
          inSlicePtr += inInc[dimIndex];
          }

        rowPtr = rowBuffer;
        if (operation == VTK_PROJECTION_AVERAGE)
          {
          // do the division via multiplication
          double factor = 1.0/numSlices;
          for (int k = 0; k < rowlen; k++)
            {
            vtkProjectionRound((*rowPtr++)*factor, *outPtr++);
            }
          }
        else // VTK_PROJECTION_SUM
          {
          // clamp to limits of numeric type
          for (int k = 0; k < rowlen; k++)
            {
            vtkProjectionClamp(*rowPtr++, *outPtr++);
            }
          }
        }

      // ====== code for handling max and min ======
      else
        {
        T1 *inSlicePtr = inPtrY;
        T2 *outPtrX = outPtr;

        // initialize using first row
        T1 *inPtrX = inSlicePtr;
        for (int j = 0; j < rowlen; j++)
          {
          *outPtrX++ = *inPtrX++;
          }
        inSlicePtr += inInc[dimIndex];

        if (operation == VTK_PROJECTION_MINIMUM)
          {
          for (int sliceIdx = 1; sliceIdx < numSlices; sliceIdx++)
            {
            inPtrX = inSlicePtr;
            outPtrX = outPtr;

            for (int i = 0; i < rowlen; i++)
              {
              // branch prediction: most often, output is not changed
              T2 inVal = *inPtrX++;
              T2 outVal = *outPtrX;
              if (inVal > outVal)
                {
                outPtrX++;
                continue;
                }
              *outPtrX++ = inVal;
              }

            inSlicePtr += inInc[dimIndex];
            }
          }
        else // VTK_PROJECTION_MAXIMUM
          {
          for (int sliceIdx = 1; sliceIdx < numSlices; sliceIdx++)
            {
            inPtrX = inSlicePtr;
            outPtrX = outPtr;

            for (int i = 0; i < rowlen; i++)
              {
              // branch prediction: most often, output is not changed
              T2 inVal = *inPtrX++;
              T2 outVal = *outPtrX;
              if (inVal < outVal)
                {
                outPtrX++;
                continue;
                }
              *outPtrX++ = inVal;
              }

            inSlicePtr += inInc[dimIndex];
            }
          }

        outPtr += rowlen;
        }

      // ====== end of operation-specific code ======

      outPtr += outIncY;
      inPtrY += inInc[1];
      }

    outPtr += outIncZ;
    inPtr += inInc[2];
    }

  if (operation == VTK_PROJECTION_AVERAGE ||
      operation == VTK_PROJECTION_SUM)
    {
    delete [] rowBuffer;
    }
}


//----------------------------------------------------------------------------
void vtkImageProjection::ThreadedRequestData(vtkInformation *,
  vtkInformationVector **inVector, vtkInformationVector *,
  vtkImageData ***inData, vtkImageData **outData, int outExt[6], int id)
{
  void *inPtr;
  void *outPtr;
  int inExt[6];
  int extent[6];
  int dimIndex;
  int range[2];

  vtkDebugMacro("Execute: inData = " << inData << ", outData = " << outData);

  // get the direction along which to sum slices
  dimIndex = this->GetSliceDirection();

  // clamp the range to the whole extent
  vtkInformation *inInfo = inVector[0]->GetInformationObject(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  this->GetSliceRange(range);
  if (range[0] < extent[2*dimIndex])
    {
    range[0] = extent[2*dimIndex];
    }
  if (range[1] > extent[2*dimIndex+1])
    {
    range[1] = extent[2*dimIndex+1];
    }

  // initialize input extent to output extent
  inExt[0] = outExt[0];
  inExt[1] = outExt[1];
  inExt[2] = outExt[2];
  inExt[3] = outExt[3];
  inExt[4] = outExt[4];
  inExt[5] = outExt[5];

  // the adjust for the slice range
  inExt[2*dimIndex] += range[0];
  inExt[2*dimIndex+1] += range[1];

  // now get the pointers for the extents
  inPtr = inData[0][0]->GetScalarPointerForExtent(inExt);
  outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  // get the scalar type
  int outScalarType = outData[0]->GetScalarType();
  int inScalarType = inData[0][0]->GetScalarType();

  // and call the execute method
  if (outScalarType == inScalarType)
    {
    switch (inScalarType)
      {
      vtkTemplateMacro(
        vtkImageProjectionExecute(this,
          inData[0][0], static_cast<VTK_TT *>(inPtr),
          outData[0], static_cast<VTK_TT *>(outPtr), outExt, id));
      default:
        vtkErrorMacro("Execute: Unknown ScalarType");
        return;
      }
    }
  else if (outScalarType == VTK_FLOAT)
    {
    switch (inScalarType)
      {
      vtkTemplateMacro(
        vtkImageProjectionExecute( this,
          inData[0][0], static_cast<VTK_TT *>(inPtr),
          outData[0], static_cast<float *>(outPtr), outExt, id));
      default:
        vtkErrorMacro("Execute: Unknown ScalarType");
        return;
      }
    }
  else if (outScalarType == VTK_DOUBLE)
    {
    switch (inScalarType)
      {
      vtkTemplateMacro(
        vtkImageProjectionExecute(this,
          inData[0][0], static_cast<VTK_TT *>(inPtr),
          outData[0], static_cast<double *>(outPtr), outExt, id));
      default:
        vtkErrorMacro("Execute: Unknown ScalarType");
        return;
      }
    }
  else
    {
    vtkErrorMacro("Execute: Unknown ScalarType");
    return;
    }
}

//----------------------------------------------------------------------------
void vtkImageProjection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Operation: " << this->GetOperationAsString() << "\n";
  os << indent << "SliceDirection: " << this->GetSliceDirection() << "\n";
  os << indent << "SliceRange: " << this->GetSliceRange()[0] << " "
     << this->GetSliceRange()[1] << "\n";
  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  os << indent << "MultiSliceOutput: "
     << (this->MultiSliceOutput ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
const char *vtkImageProjection::GetOperationAsString()
{
  switch (this->Operation)
    {
    case VTK_PROJECTION_AVERAGE:
      return "Average";
    case VTK_PROJECTION_SUM:
      return "Sum";
    case VTK_PROJECTION_MINIMUM:
      return "Minimum";
    case VTK_PROJECTION_MAXIMUM:
      return "Maximum";
    default:
      return "";
    }
}
