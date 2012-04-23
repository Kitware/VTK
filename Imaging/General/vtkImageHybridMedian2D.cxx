/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHybridMedian2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageHybridMedian2D.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include <vector>
#include <algorithm>
#include <numeric>

vtkStandardNewMacro(vtkImageHybridMedian2D);

//----------------------------------------------------------------------------
vtkImageHybridMedian2D::vtkImageHybridMedian2D()
{
  this->KernelSize[0] = 5;
  this->KernelSize[1] = 5;
  this->KernelSize[2] = 1;
  this->KernelMiddle[0] = 2;
  this->KernelMiddle[1] = 2;
  this->KernelMiddle[2] = 0;
  this->HandleBoundaries = 1;
}

template <class T>
void vtkImageHybridMedian2DExecute(vtkImageHybridMedian2D *self,
                                   vtkImageData *inData, T *inPtr2,
                                   vtkImageData *outData, T *outPtr2,
                                   int outExt[6], int id,
                                   vtkInformation *inInfo)
{
  int idx0, idx1, idx2, idxC;
  vtkIdType inInc0, inInc1, inInc2;
  vtkIdType outInc0, outInc1, outInc2;
  int min0, max0, min1, max1, min2, max2, numComps;
  int wholeMin0, wholeMax0, wholeMin1, wholeMax1;
  int wholeExt[6];
  T *inPtr0, *inPtr1, *inPtrC;
  T *outPtr0, *outPtr1, *outPtrC, *ptr;
  T median1, median2, temp;
  std::vector<T> array;
  unsigned long count = 0;
  unsigned long target;

  inData->GetIncrements(inInc0, inInc1, inInc2);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);
  wholeMin0 = wholeExt[0];
  wholeMax0 = wholeExt[1];
  wholeMin1 = wholeExt[2];
  wholeMax1 = wholeExt[3];
  numComps = inData->GetNumberOfScalarComponents();
  outData->GetIncrements(outInc0, outInc1, outInc2);
  min0 = outExt[0];   max0 = outExt[1];
  min1 = outExt[2];   max1 = outExt[3];
  min2 = outExt[4];   max2 = outExt[5];

  target = static_cast<unsigned long>((max2-min2+1)*(max1-min1+1)/50.0);
  target++;

  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;

    for (idx1 = min1; !self->AbortExecute && idx1 <= max1; ++idx1)
      {
      if (!id)
        {
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      inPtr0 = inPtr1;
      outPtr0 = outPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
        {
        inPtrC = inPtr0;
        outPtrC = outPtr0;
        for (idxC = 0; idxC < numComps; ++idxC)
          {
          // compute median of + neighborhood
          // note that y axis direction is up in vtk images, not down
          // as in screen coordinates
          array.clear();
          // Center
          ptr = inPtrC;
          array.push_back( *ptr );
          // left
          ptr = inPtrC;
          if (idx0 > wholeMin0)
            {
            ptr -= inInc0;
            array.push_back( *ptr );
            }
          if (idx0 - 1 > wholeMin0)
            {
            ptr -= inInc0;
            array.push_back( *ptr );
            }
          // right
          ptr = inPtrC;
          if (idx0 < wholeMax0)
            {
            ptr += inInc0;
            array.push_back( *ptr );
            }
          if (idx0 + 1 < wholeMax0)
            {
            ptr += inInc0;
            array.push_back( *ptr );
            }
          // down
          ptr = inPtrC;
          if (idx1 > wholeMin1)
            {
            ptr -= inInc1;
            array.push_back( *ptr );
            }
          if (idx1 - 1 > wholeMin1)
            {
            ptr -= inInc1;
            array.push_back( *ptr );
            }
          // up
          ptr = inPtrC;
          if (idx1 < wholeMax1)
            {
            ptr += inInc1;
            array.push_back( *ptr );
            }
          if (idx1 + 1 < wholeMax1)
            {
            ptr += inInc1;
            array.push_back( *ptr );
            }

          std::sort(array.begin(),array.end());
          median1 = array[static_cast<unsigned int>(0.5*array.size())];

          // compute median of x neighborhood
          // note that y axis direction is up in vtk images, not down
          // as in screen coordinates
          array.clear();
          // Center
          ptr = inPtrC;
          array.push_back( *ptr );
          // lower left
          if (idx0 > wholeMin0 && idx1 > wholeMin1)
            {
            ptr -= inInc0 + inInc1;
            array.push_back( *ptr );
            }
          if (idx0-1 > wholeMin0 && idx1-1 > wholeMin1)
            {
            ptr -= inInc0 + inInc1;
            array.push_back( *ptr );
            }
          // upper right
          ptr = inPtrC;
          if (idx0 < wholeMax0 && idx1 < wholeMax1)
            {
            ptr += inInc0 + inInc1;
            array.push_back( *ptr );
            }
          if (idx0+1 < wholeMax0 && idx1+1 < wholeMax1)
            {
            ptr += inInc0 + inInc1;
            array.push_back( *ptr );
            }
          // upper left
          ptr = inPtrC;
          if (idx0 > wholeMin0 && idx1 < wholeMax1)
            {
            ptr += -inInc0 + inInc1;
            array.push_back( *ptr );
            }
          if (idx0-1 > wholeMin0 && idx1+1 < wholeMax1)
            {
            ptr += -inInc0 + inInc1;
            array.push_back( *ptr );
            }
          // lower right
          ptr = inPtrC;
          if (idx0 < wholeMax0 && idx1 > wholeMin1)
            {
            ptr += inInc0 - inInc1;
            array.push_back( *ptr );
            }
          if (idx0+1 < wholeMax0 && idx1-1 > wholeMin1)
            {
            ptr += inInc0 - inInc1;
            array.push_back( *ptr );
            }

          std::sort(array.begin(),array.end());
          median2 = array[static_cast<unsigned int>(0.5*array.size())];

          // Compute the median of the three. (med1, med2 and center)
          if (median1 > median2)
            {
            temp = median1;
            median1 = median2;
            median2 = temp;
            }
          if (*inPtrC < median1)
            {
            *outPtrC = median1;
            }
          else if (*inPtrC < median2)
            {
            *outPtrC = *inPtrC;
            }
          else
            {
            *outPtrC = median2;
            }
          ++inPtrC;
          ++outPtrC;
          }
        inPtr0 += inInc0;
        outPtr0 += outInc0;
        }
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
    }
}

// This method contains the first switch statement that calls the correct
// templated function for the input and output Data types.
// It hanldes image boundaries, so the image does not shrink.
void vtkImageHybridMedian2D::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  void *inPtr = inData[0][0]->GetScalarPointerForExtent(outExt);
  void *outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // this filter expects the output type to be same as input
  if (outData[0]->GetScalarType() != inData[0][0]->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: output ScalarType, "
      << vtkImageScalarTypeNameMacro(outData[0]->GetScalarType())
      << " must match input scalar type");
    return;
    }

  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageHybridMedian2DExecute( this, inData[0][0],
                                     static_cast<VTK_TT *>(inPtr), outData[0],
                                     static_cast<VTK_TT *>(outPtr),
                                     outExt, id, inInfo));

    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
