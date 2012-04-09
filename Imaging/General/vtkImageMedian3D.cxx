/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMedian3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageMedian3D.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageMedian3D);

//-----------------------------------------------------------------------------
// Construct an instance of vtkImageMedian3D fitler.
vtkImageMedian3D::vtkImageMedian3D()
{
  this->NumberOfElements = 0;
  this->SetKernelSize(1,1,1);
  this->HandleBoundaries = 1;
}

//-----------------------------------------------------------------------------
vtkImageMedian3D::~vtkImageMedian3D()
{
}

//-----------------------------------------------------------------------------
void vtkImageMedian3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfElements: " << this->NumberOfElements << endl;
}

//-----------------------------------------------------------------------------
// This method sets the size of the neighborhood.  It also sets the
// default middle of the neighborhood
void vtkImageMedian3D::SetKernelSize(int size0, int size1, int size2)
{
  int volume;
  int modified = 1;

  if (this->KernelSize[0] == size0 && this->KernelSize[1] == size1 &&
      this->KernelSize[2] == size2)
    {
    modified = 0;
    }

  // Set the kernel size and middle
  volume = 1;
  this->KernelSize[0] = size0;
  this->KernelMiddle[0] = size0 / 2;
  volume *= size0;
  this->KernelSize[1] = size1;
  this->KernelMiddle[1] = size1 / 2;
  volume *= size1;
  this->KernelSize[2] = size2;
  this->KernelMiddle[2] = size2 / 2;
  volume *= size2;

  this->NumberOfElements = volume;
  if ( modified )
    {
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
// Add a sample to the median computation
double *vtkImageMedian3DAccumulateMedian(int &UpNum, int &DownNum,
                                         int &UpMax, int &DownMax,
                                         int &NumNeighborhood,
                                         double *Median, double val)
{
  int idx, max;
  double temp, *ptr;

  // special case: no samples yet
  if (UpNum == 0)
    {
    *(Median) = val;
    // length of up and down arrays inclusive of current
    UpNum = DownNum = 1;
    // median is guaranteed to be in this range (length of array)
    DownMax = UpMax = (NumNeighborhood + 1) / 2;
    return Median;
    }

  // Case: value is above median
  if (val >= *(Median))
    {
    // move the median if necessary
    if (UpNum > DownNum)
      {
      // Move the median Up one
      ++Median;
      --UpNum;
      ++DownNum;
      --UpMax;
      ++DownMax;
      }
    // find the position for val in the sorted array
    max = (UpNum < UpMax) ? UpNum : UpMax;
    ptr = Median;
    idx = 0;
    while (idx < max && val >= *ptr)
      {
      ++ptr;
      ++idx;
      }
    // place val and move all others up
    while (idx < max)
      {
      temp = *ptr;
      *ptr = val;
      val = temp;
      ++ptr;
      ++idx;
      }
    *ptr = val;
    // Update counts
    ++UpNum;
    --DownMax;
    return Median;
    }

  // Case: value is below median
  // If we got here, val < *(Median)

  // move the median if necessary
  if (DownNum > UpNum)
    {
    // Move the median Down one
    --Median;
    --DownNum;
    ++UpNum;
    --DownMax;
    ++UpMax;
    }
  // find the position for val in the sorted array
  max = (DownNum < DownMax) ? DownNum : DownMax;
  ptr = Median;
  idx = 0;
  while (idx < max && val <= *ptr)
    {
    --ptr;
    ++idx;
    }
  // place val and move all others up
  while (idx < max)
    {
    temp = *ptr;
    *ptr = val;
    val = temp;
    --ptr;
    ++idx;
    }
  *ptr = val;
  // Update counts
  ++DownNum;
  --UpMax;

  return Median;
}

//-----------------------------------------------------------------------------
// This method contains the second switch statement that calls the correct
// templated function for the mask types.
template <class T>
void vtkImageMedian3DExecute(vtkImageMedian3D *self,
                             vtkImageData *inData, T *inPtr,
                             vtkImageData *outData, T *outPtr,
                             int outExt[6], int id,
                             vtkDataArray *inArray)
{
  int *kernelMiddle, *kernelSize;
  int NumberOfElements;
  // For looping though output (and input) pixels.
  int outIdx0, outIdx1, outIdx2;
  vtkIdType inInc0, inInc1, inInc2;
  int outIdxC;
  vtkIdType outIncX, outIncY, outIncZ;
  T *inPtr0, *inPtr1, *inPtr2;
  // For looping through hood pixels
  int hoodMin0, hoodMax0, hoodMin1, hoodMax1, hoodMin2, hoodMax2;
  int hoodStartMin0, hoodStartMax0, hoodStartMin1, hoodStartMax1;
  int hoodIdx0, hoodIdx1, hoodIdx2;
  T *tmpPtr0, *tmpPtr1, *tmpPtr2;
  // The portion of the out image that needs no boundary processing.
  int middleMin0, middleMax0, middleMin1, middleMax1, middleMin2, middleMax2;
  int numComp;
  // variables for the median calc
  int UpNum = 0;
  int DownNum = 0;
  int UpMax = 0;
  int DownMax = 0;
  double *Median;
  int *inExt;
  unsigned long count = 0;
  unsigned long target;

  if (!inArray)
    {
    return;
    }

  double *Sort = new double[(self->GetNumberOfElements() + 8)];

  // Get information to march through data
  inData->GetIncrements(inInc0, inInc1, inInc2);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  kernelMiddle = self->GetKernelMiddle();
  kernelSize = self->GetKernelSize();

  numComp = inArray->GetNumberOfComponents();

  hoodMin0 = outExt[0] - kernelMiddle[0];
  hoodMin1 = outExt[2] - kernelMiddle[1];
  hoodMin2 = outExt[4] - kernelMiddle[2];
  hoodMax0 = kernelSize[0] + hoodMin0 - 1;
  hoodMax1 = kernelSize[1] + hoodMin1 - 1;
  hoodMax2 = kernelSize[2] + hoodMin2 - 1;

  // Clip by the input image extent
  inExt = inData->GetExtent();
  hoodMin0 = (hoodMin0 > inExt[0]) ? hoodMin0 : inExt[0];
  hoodMin1 = (hoodMin1 > inExt[2]) ? hoodMin1 : inExt[2];
  hoodMin2 = (hoodMin2 > inExt[4]) ? hoodMin2 : inExt[4];
  hoodMax0 = (hoodMax0 < inExt[1]) ? hoodMax0 : inExt[1];
  hoodMax1 = (hoodMax1 < inExt[3]) ? hoodMax1 : inExt[3];
  hoodMax2 = (hoodMax2 < inExt[5]) ? hoodMax2 : inExt[5];

  // Save the starting neighborhood dimensions (2 loops only once)
  hoodStartMin0 = hoodMin0;    hoodStartMax0 = hoodMax0;
  hoodStartMin1 = hoodMin1;    hoodStartMax1 = hoodMax1;

  // The portion of the output that needs no boundary computation.
  middleMin0 = inExt[0] + kernelMiddle[0];
  middleMax0 = inExt[1] - (kernelSize[0] - 1) + kernelMiddle[0];
  middleMin1 = inExt[2] + kernelMiddle[1];
  middleMax1 = inExt[3] - (kernelSize[1] - 1) + kernelMiddle[1];
  middleMin2 = inExt[4] + kernelMiddle[2];
  middleMax2 = inExt[5] - (kernelSize[2] - 1) + kernelMiddle[2];

  target = static_cast<unsigned long>((outExt[5] - outExt[4] + 1)*
                                      (outExt[3] - outExt[2] + 1)/50.0);
  target++;

  NumberOfElements = self->GetNumberOfElements();

  // loop through pixel of output
  inPtr = static_cast<T *>(
    inArray->GetVoidPointer((hoodMin0 - inExt[0])* inInc0 +
                            (hoodMin1 - inExt[2])* inInc1 +
                            (hoodMin2 - inExt[4])* inInc2));
  inPtr2 = inPtr;
  for (outIdx2 = outExt[4]; outIdx2 <= outExt[5]; ++outIdx2)
    {
    inPtr1 = inPtr2;
    hoodMin1 = hoodStartMin1;
    hoodMax1 = hoodStartMax1;
    for (outIdx1 = outExt[2];
         !self->AbortExecute && outIdx1 <= outExt[3]; ++outIdx1)
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
      hoodMin0 = hoodStartMin0;
      hoodMax0 = hoodStartMax0;
      for (outIdx0 = outExt[0]; outIdx0 <= outExt[1]; ++outIdx0)
        {
        for (outIdxC = 0; outIdxC < numComp; outIdxC++)
          {
          // Compute median of neighborhood
          // Note: For boundary, NumNeighborhood could be changed for
          // a faster sort.
          DownNum = UpNum = 0;
          Median = Sort + (NumberOfElements / 2) + 4;
          // loop through neighborhood pixels
          tmpPtr2 = inPtr0 + outIdxC;
          for (hoodIdx2 = hoodMin2; hoodIdx2 <= hoodMax2; ++hoodIdx2)
            {
            tmpPtr1 = tmpPtr2;
            for (hoodIdx1 = hoodMin1; hoodIdx1 <= hoodMax1; ++hoodIdx1)
              {
              tmpPtr0 = tmpPtr1;
              for (hoodIdx0 = hoodMin0; hoodIdx0 <= hoodMax0; ++hoodIdx0)
                {
                // Add this pixel to the median
                Median = vtkImageMedian3DAccumulateMedian(UpNum, DownNum,
                                                          UpMax, DownMax,
                                                          NumberOfElements,
                                                          Median,
                                                          double(*tmpPtr0));

                tmpPtr0 += inInc0;
                }
              tmpPtr1 += inInc1;
              }
            tmpPtr2 += inInc2;
            }

          // Replace this pixel with the hood median
          *outPtr = static_cast<T>(*Median);
          outPtr++;
          }

        // shift neighborhood considering boundaries
        if (outIdx0 >= middleMin0)
          {
          inPtr0 += inInc0;
          ++hoodMin0;
          }
        if (outIdx0 < middleMax0)
          {
          ++hoodMax0;
          }
        }
      // shift neighborhood considering boundaries
      if (outIdx1 >= middleMin1)
        {
        inPtr1 += inInc1;
        ++hoodMin1;
        }
      if (outIdx1 < middleMax1)
        {
        ++hoodMax1;
        }
      outPtr += outIncY;
      }
    // shift neighborhood considering boundaries
    if (outIdx2 >= middleMin2)
      {
      inPtr2 += inInc2;
      ++hoodMin2;
      }
    if (outIdx2 < middleMax2)
      {
      ++hoodMax2;
      }
    outPtr += outIncZ;
    }

  delete [] Sort;
}

//-----------------------------------------------------------------------------
// This method contains the first switch statement that calls the correct
// templated function for the input and output region types.
void vtkImageMedian3D::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  void *inPtr;
  void *outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  vtkDataArray *inArray = this->GetInputArrayToProcess(0,inputVector);
  if (id == 0)
    {
    outData[0]->GetPointData()->GetScalars()->SetName(inArray->GetName());
    }

  inPtr = inArray->GetVoidPointer(0);

  // this filter expects that input is the same type as output.
  if (inArray->GetDataType() != outData[0]->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input data type, " << inArray->GetDataType()
                << ", must match out ScalarType "
                  << outData[0]->GetScalarType());
    return;
    }

  switch (inArray->GetDataType())
    {
    vtkTemplateMacro(
      vtkImageMedian3DExecute(this,inData[0][0],
                              static_cast<VTK_TT *>(inPtr),
                              outData[0], static_cast<VTK_TT *>(outPtr),
                              outExt, id,inArray));
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}
