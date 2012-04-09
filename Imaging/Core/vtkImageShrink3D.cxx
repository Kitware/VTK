/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShrink3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageShrink3D.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageShrink3D);

//----------------------------------------------------------------------------
// Constructor: Sets default filter to be identity.
vtkImageShrink3D::vtkImageShrink3D()
{
  this->ShrinkFactors[0] = this->ShrinkFactors[1] = this->ShrinkFactors[2] = 1;
  this->Shift[0] = this->Shift[1] = this->Shift[2] = 0;
  this->Mean = 1;
  this->Median = 0;
  this->Maximum = 0;
  this->Minimum = 0;
}

void vtkImageShrink3D::SetMean (int value)
{
  if (value != this->Mean)
    {
    this->Mean = value;
    if (value == 1)
      {
      this->Minimum = 0;
      this->Maximum = 0;
      this->Median = 0;
      }      
    this->Modified();
    }
}

void vtkImageShrink3D::SetMinimum (int value)
{
  if (value != this->Minimum)
    {
    this->Minimum = value;
    if (value == 1)
      {
      this->Mean = 0;
      this->Maximum = 0;
      this->Median = 0;
      }      
    this->Modified();
    }
}

void vtkImageShrink3D::SetMaximum (int value)
{
  if (value != this->Maximum)
    {
    this->Maximum = value;
    if (value == 1)
      {
      this->Minimum = 0;
      this->Mean = 0;
      this->Median = 0;
      }      
    this->Modified();
    }
}

void vtkImageShrink3D::SetMedian (int value)
{
  if (value != this->Median)
    {
    this->Median = value;
    if (value == 1)
      {
      this->Minimum = 0;
      this->Maximum = 0;
      this->Mean = 0;
      }      
    this->Modified();
    }
}

void vtkImageShrink3D::SetAveraging (int value)
{
  this->SetMean(value);
}

//----------------------------------------------------------------------------
void vtkImageShrink3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ShrinkFactors: (" << this->ShrinkFactors[0] << ", "
     << this->ShrinkFactors[1] << ", " << this->ShrinkFactors[2] << ")\n";
  os << indent << "Shift: (" << this->Shift[0] << ", "
     << this->Shift[1] << ", " << this->Shift[2] << ")\n";

  os << indent << "Averaging: " << (this->Mean ? "On\n" : "Off\n");
  os << indent << "Mean: " << (this->Mean ? "On\n" : "Off\n");
  os << indent << "Minimum: " << (this->Minimum ? "On\n" : "Off\n");
  os << indent << "Maximum: " << (this->Maximum ? "On\n" : "Off\n");
  os << indent << "Median: " << (this->Median ? "On\n" : "Off\n");
}

void vtkImageShrink3D::InternalRequestUpdateExtent(int *inExt, int *outExt)
{
  int idx;
  
  for (idx = 0; idx < 3; ++idx)
    {
    // For Min.
    inExt[idx*2] = outExt[idx*2] * this->ShrinkFactors[idx] 
      + this->Shift[idx];
    // For Max.
    inExt[idx*2+1] = outExt[idx*2+1] * this->ShrinkFactors[idx]
      + this->Shift[idx];
    // If we are not sub sampling, we need a little more
    if (this->Mean || this->Minimum || this->Maximum || this->Median)
      {
      inExt[idx*2+1] += this->ShrinkFactors[idx] - 1;
      }
    }
}


//----------------------------------------------------------------------------
// This method computes the Region of input necessary to generate outRegion.
int vtkImageShrink3D::RequestUpdateExtent (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  int outExt[6], inExt[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExt);

  this->InternalRequestUpdateExtent(inExt, outExt);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  return 1;
}

//----------------------------------------------------------------------------
// Computes any global image information associated with regions.
// Any problems with roundoff or negative numbers ???
int vtkImageShrink3D::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  int idx;
  int wholeExtent[6];
  double spacing[3];

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExtent);
  inInfo->Get(vtkDataObject::SPACING(), spacing);

  for (idx = 0; idx < 3; ++idx)
    {
    // Avoid dividing by 0.
      if (this->ShrinkFactors[idx] == 0)
      {
      this->ShrinkFactors[idx] = 1;
      }
    // Scale the output extent
    wholeExtent[2*idx] = 
      static_cast<int>(ceil(static_cast<double>(wholeExtent[2*idx] - this->Shift[idx]) 
                 / static_cast<double>(this->ShrinkFactors[idx])));
    wholeExtent[2*idx+1] = static_cast<int>(floor(
     static_cast<double>(wholeExtent[2*idx+1]-this->Shift[idx]-this->ShrinkFactors[idx]+1)
         / static_cast<double>(this->ShrinkFactors[idx])));
     // make sure WholeExtent is valid when the ShrinkFactors are set on an
     // axis with no Extent beforehand
     if (wholeExtent[2*idx+1]<wholeExtent[2*idx])
       {
       wholeExtent[2*idx+1] = wholeExtent[2*idx];
       }
    // Change the data spacing
    spacing[idx] *= static_cast<double>(this->ShrinkFactors[idx]);
    }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExtent,6);
  outInfo->Set(vtkDataObject::SPACING(),spacing,3);

  return 1;
}

template <class T>
#ifdef _WIN32_WCE
int __cdecl vtkiscompare(const T *y1,const T *y2)
#else
int vtkiscompare(const T *y1,const T *y2)
#endif
{
  if ( *y1 <  *y2) 
    {
    return -1; 
    }
  
  if ( *y1 == *y2) 
    {
    return  0;
    }
  
  return  1;
}

extern "C" 
{
  typedef int (*vtkCompareFunction)(const void*, const void*);
}

//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
template <class T>
void vtkImageShrink3DExecute(vtkImageShrink3D *self,
                             vtkImageData *inData, T *inPtr,
                             vtkImageData *outData, T *outPtr,
                             int outExt[6], int id,
                             vtkInformation *inInfo)
{
  int outIdx0, outIdx1, outIdx2, inIdx0, inIdx1, inIdx2;
  vtkIdType inInc0, inInc1, inInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  vtkIdType outInc0, outInc1, outInc2;
  vtkIdType tmpInc0, tmpInc1, tmpInc2;
  T *tmpPtr0, *tmpPtr1, *tmpPtr2;
  int factor0, factor1, factor2;
  double sum, norm;
  unsigned long count = 0;
  unsigned long target;
  int idxC, maxC, maxX;
  T *outPtr2;

  // black magic to force the correct version of the comparison function
  // to be instantiated AND used.
#ifdef _WIN32_WCE
  int (__cdecl *compareF1)(const T*, const T*) = vtkiscompare;
  int (__cdecl *compareFn)(const void*, const void*)
    = (int (__cdecl *)(const void*, const void*)) compareF1;
#else
  int (*compareF1)(const T*, const T*) = vtkiscompare;
//  int (*compareFn)(const void*, const void*)
//    = (int (*)(const void*, const void*)) compareF1;
  vtkCompareFunction compareFn = 
    reinterpret_cast<vtkCompareFunction>(compareF1);
#endif

  self->GetShrinkFactors(factor0, factor1, factor2);

  // make sure we don't have a 3D shrinkfactor for a 2D image
  if (factor2>1 && inData && inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT())[5]==0)
    {
    factor2=1;
    }

  // Get information to march through data
  inData->GetIncrements(inInc0, inInc1, inInc2);
  tmpInc0 = inInc0 * factor0;
  tmpInc1 = inInc1 * factor1;
  tmpInc2 = inInc2 * factor2;
  outData->GetContinuousIncrements(outExt,outInc0, outInc1, outInc2);

  maxX = outExt[1] - outExt[0];
  maxC = inData->GetNumberOfScalarComponents();
  target = static_cast<unsigned long>(maxC*(outExt[5] - outExt[4] + 1)*
                                      (outExt[3] - outExt[2] + 1)/50.0);
  target++;
  
  if (self->GetMean())
    {
    norm = 1.0 / static_cast<double>(factor0 * factor1 * factor2);
    // Loop through output pixels
    for (idxC = 0; idxC < maxC; idxC++)
      {
      tmpPtr2 = inPtr + idxC;
      outPtr2 = outPtr + idxC;
      for (outIdx2 = outExt[4]; outIdx2 <= outExt[5]; ++outIdx2)
        {
        tmpPtr1 = tmpPtr2;
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
          tmpPtr0 = tmpPtr1;
          for (outIdx0 = 0; outIdx0 <= maxX; ++outIdx0)
            {
            sum = 0.0;
            // Loop through neighborhood pixels
            inPtr2 = tmpPtr0;
            for (inIdx2 = 0; inIdx2 < factor2; ++inIdx2)
              {
              inPtr1 = inPtr2;
              for (inIdx1 = 0; inIdx1 < factor1; ++inIdx1)
                {
                inPtr0 = inPtr1;
                for (inIdx0 = 0; inIdx0 < factor0; ++inIdx0)
                  {
                  sum += static_cast<double>(*inPtr0);
                  inPtr0 += inInc0;
                  }
                inPtr1 += inInc1;
                }
              inPtr2 += inInc2;
              }
            *outPtr2 = static_cast<T>(sum * norm);
            tmpPtr0 += tmpInc0;
            outPtr2 += maxC;
            }
          tmpPtr1 += tmpInc1;
          outPtr2 += outInc1;
          }
        tmpPtr2 += tmpInc2;
        outPtr2 += outInc2;
        }
      }
    }
  else if (self->GetMinimum())
    {
    T minValue;
    // Loop through output pixels
    for (idxC = 0; idxC < maxC; idxC++)
      {
      tmpPtr2 = inPtr + idxC;
      outPtr2 = outPtr + idxC;
      for (outIdx2 = outExt[4]; outIdx2 <= outExt[5]; ++outIdx2)
        {
        tmpPtr1 = tmpPtr2;
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
          tmpPtr0 = tmpPtr1;
          for (outIdx0 = 0; outIdx0 <= maxX; ++outIdx0)
            {
            minValue = static_cast<T>(self->GetOutput()->GetScalarTypeMax());
            // Loop through neighborhood pixels
            inPtr2 = tmpPtr0;
            for (inIdx2 = 0; inIdx2 < factor2; ++inIdx2)
              {
              inPtr1 = inPtr2;
              for (inIdx1 = 0; inIdx1 < factor1; ++inIdx1)
                {
                inPtr0 = inPtr1;
                for (inIdx0 = 0; inIdx0 < factor0; ++inIdx0)
                  {
                  if (*inPtr0 < minValue)
                    {
                    minValue = *inPtr0;
                    }
                  inPtr0 += inInc0;
                  }
                inPtr1 += inInc1;
                }
              inPtr2 += inInc2;
              }
            *outPtr2 = minValue;
            tmpPtr0 += tmpInc0;
            outPtr2 += maxC;
            }
          tmpPtr1 += tmpInc1;
          outPtr2 += outInc1;
          }
        tmpPtr2 += tmpInc2;
        outPtr2 += outInc2;
        }
      }
    }
  else if (self->GetMaximum())
    {
    T maxValue;
    // Loop through output pixels
    for (idxC = 0; idxC < maxC; idxC++)
      {
      tmpPtr2 = inPtr + idxC;
      outPtr2 = outPtr + idxC;
      for (outIdx2 = outExt[4]; outIdx2 <= outExt[5]; ++outIdx2)
        {
        tmpPtr1 = tmpPtr2;
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
          tmpPtr0 = tmpPtr1;
          for (outIdx0 = 0; outIdx0 <= maxX; ++outIdx0)
            {
            maxValue = static_cast<T>(self->GetOutput()->GetScalarTypeMin());
            // Loop through neighborhood pixels
            inPtr2 = tmpPtr0;
            for (inIdx2 = 0; inIdx2 < factor2; ++inIdx2)
              {
              inPtr1 = inPtr2;
              for (inIdx1 = 0; inIdx1 < factor1; ++inIdx1)
                {
                inPtr0 = inPtr1;
                for (inIdx0 = 0; inIdx0 < factor0; ++inIdx0)
                  {
                  if (*inPtr0 > maxValue)
                    {
                    maxValue = *inPtr0;
                    }
                  inPtr0 += inInc0;
                  }
                inPtr1 += inInc1;
                }
              inPtr2 += inInc2;
              }
            *outPtr2 = maxValue;
            tmpPtr0 += tmpInc0;
            outPtr2 += maxC;
            }
          tmpPtr1 += tmpInc1;
          outPtr2 += outInc1;
          }
        tmpPtr2 += tmpInc2;
        outPtr2 += outInc2;
        }
      }
    }
  else if (self->GetMedian())
    {
    T* kernel = new T [factor0 * factor1 * factor2];
    int index;
    
    // Loop through output pixels
    for (idxC = 0; idxC < maxC; idxC++)
      {
      tmpPtr2 = inPtr + idxC;
      outPtr2 = outPtr + idxC;
      for (outIdx2 = outExt[4]; outIdx2 <= outExt[5]; ++outIdx2)
        {
        tmpPtr1 = tmpPtr2;
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
          tmpPtr0 = tmpPtr1;
          for (outIdx0 = 0; outIdx0 <= maxX; ++outIdx0)
            {
            // Loop through neighborhood pixels
            inPtr2 = tmpPtr0;
            index = 0;
            for (inIdx2 = 0; inIdx2 < factor2; ++inIdx2)
              {
              inPtr1 = inPtr2;
              for (inIdx1 = 0; inIdx1 < factor1; ++inIdx1)
                {
                inPtr0 = inPtr1;
                for (inIdx0 = 0; inIdx0 < factor0; ++inIdx0)
                  {
                  kernel[index++] = *inPtr0;

                  inPtr0 += inInc0;
                  }
                inPtr1 += inInc1;
                }
              inPtr2 += inInc2;
              }
            qsort(kernel,index,sizeof(T),compareFn);
            *outPtr2 = *(kernel + index/2);

            tmpPtr0 += tmpInc0;
            outPtr2 += maxC;
            }
          tmpPtr1 += tmpInc1;
          outPtr2 += outInc1;
          }
        tmpPtr2 += tmpInc2;
        outPtr2 += outInc2;
        }
      }
    delete [] kernel;
    }
  else // Just SubSample
    {
    // Loop through output pixels
    for (idxC = 0; idxC < maxC; idxC++)
      {
      tmpPtr2 = inPtr + idxC;
      outPtr2 = outPtr + idxC;
      for (outIdx2 = outExt[4]; outIdx2 <= outExt[5]; ++outIdx2)
        {
        tmpPtr1 = tmpPtr2;
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
          tmpPtr0 = tmpPtr1;
          for (outIdx0 = 0; outIdx0 <= maxX; ++outIdx0)
            {
            *outPtr2 = *tmpPtr0;

            tmpPtr0 += tmpInc0;
            outPtr2 += maxC;
            }
          tmpPtr1 += tmpInc1;
          outPtr2 += outInc1;
          }
        tmpPtr2 += tmpInc2;
        outPtr2 += outInc2;
        }
      }
    }
}
    
//----------------------------------------------------------------------------
// This method uses the input data to fill the output data.
// It can handle any type data, but the two datas must have the same 
// data type.
void vtkImageShrink3D::ThreadedRequestData(
  vtkInformation * vtkNotUsed( request ),
  vtkInformationVector **inputVector,
  vtkInformationVector * vtkNotUsed( outputVector ),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  int inExt[6];
  void *outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  this->InternalRequestUpdateExtent(inExt, outExt);
  void *inPtr = inData[0][0]->GetScalarPointerForExtent(inExt);
  if (!inPtr)
    {
    return;
    }

  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType())
    {
    vtkErrorMacro("Execute: input ScalarType, "
                  << inData[0][0]->GetScalarType()
                  << ", must match out ScalarType "
                  << outData[0]->GetScalarType());
    return;
    }
  
  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageShrink3DExecute(this, 
                              inData[0][0],
                              static_cast<VTK_TT *>(inPtr), 
                              outData[0], 
                              static_cast<VTK_TT *>(outPtr),
                              outExt, 
                              id, 
                              inInfo));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
