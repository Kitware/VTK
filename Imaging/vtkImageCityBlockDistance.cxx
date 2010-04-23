/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCityBlockDistance.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageCityBlockDistance.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageCityBlockDistance);

//----------------------------------------------------------------------------
vtkImageCityBlockDistance::vtkImageCityBlockDistance()
{
}


//----------------------------------------------------------------------------
void vtkImageCityBlockDistance::AllocateOutputScalars(vtkImageData *outData,
                                                      int* uExt,
                                                      int* wholeExtent)
{
  int updateExtent[6], idx;
  
  memcpy(updateExtent, uExt, 6*sizeof(int));
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    updateExtent[idx*2] = wholeExtent[idx*2];
    updateExtent[idx*2+1] = wholeExtent[idx*2+1];
    }
  outData->SetExtent(updateExtent);
  outData->AllocateScalars();
}


//----------------------------------------------------------------------------
// This method tells the superclass that the whole input array is needed
// to compute any output region.
int vtkImageCityBlockDistance::IterativeRequestUpdateExtent(
  vtkInformation* input, vtkInformation* output)
{
  int *outExt = output->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  int *wExt = input->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  int inExt[6];

  memcpy(inExt, outExt, 6 * sizeof(int));
  inExt[this->Iteration * 2] = wExt[this->Iteration * 2];
  inExt[this->Iteration * 2 + 1] = wExt[this->Iteration * 2 + 1];
  input->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inExt,6);

  return 1;
}


//----------------------------------------------------------------------------
// This is writen as a 1D execute method, but is called several times.
int vtkImageCityBlockDistance::IterativeRequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkImageData *inData = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *outData = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  int *uExt = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  int *wExt = outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  this->AllocateOutputScalars(outData, uExt, wExt);
  
  short *inPtr0, *inPtr1, *inPtr2, *inPtrC;
  short *outPtr0, *outPtr1, *outPtr2, *outPtrC;
  vtkIdType inInc0, inInc1, inInc2;
  vtkIdType outInc0, outInc1, outInc2;
  int min0, max0, min1, max1, min2, max2, numberOfComponents;
  int idx0, idx1, idx2, idxC;
  short distP, distN;
  short big = 2000;
  int outExt[6];
  unsigned long count = 0;
  unsigned long target;
  
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),outExt);

  // this filter expects that inputand output are short
  if (inData->GetScalarType() != VTK_SHORT ||
      outData->GetScalarType() != VTK_SHORT)
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
                  << ", and out ScalarType " << outData->GetScalarType()
                  << " must be short.");
    return 1;
    }


  // Reorder axes (the in and out extents are assumed to be the same)
  // (see intercept cache update)
  this->PermuteExtent(outExt, min0, max0, min1, max1, min2, max2);
  this->PermuteIncrements(inData->GetIncrements(), inInc0, inInc1, inInc2);
  this->PermuteIncrements(outData->GetIncrements(), outInc0, outInc1, outInc2);
  numberOfComponents = inData->GetNumberOfScalarComponents();
  
  target = static_cast<unsigned long>((max2-min2+1)*(max1-min1+1)/50.0);
  target++;
  
  // loop over all the extra axes
  inPtr2 = static_cast<short *>(inData->GetScalarPointerForExtent(outExt));
  outPtr2 = static_cast<short *>(outData->GetScalarPointerForExtent(outExt));
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    for (idx1 = min1; !this->AbortExecute && idx1 <= max1; ++idx1)
      {
      if (!(count%target))
        {
        this->UpdateProgress(count/(50.0*target));
        }
      count++;
      inPtrC = inPtr1;
      outPtrC = outPtr1;
      for (idxC = 0; idxC < numberOfComponents; ++idxC)
        {
        // execute forward pass
        distP = big;
        distN = -big;
        inPtr0 = inPtrC;
        outPtr0 = outPtrC;
        for (idx0 = min0; idx0 <= max0; ++idx0)
          { // preserve sign
          if (*inPtr0 >= 0)
            {
            distN = 0;
            if (distP > *inPtr0)
              {
              distP = *inPtr0;
              }
            *outPtr0 = distP;
            }
          if (*inPtr0 <= 0)
            {
            distP = 0;
            if (distN < *inPtr0)
              {
              distN = *inPtr0;
              }
            *outPtr0 = distN;
            }
          
          if (distP < big)
            {
            ++distP;
            }
          if (distN > -big)
            {
            --distN;
            }
          
          inPtr0 += inInc0;
          outPtr0 += outInc0;
          }
        
        // backward pass
        distP = big;
        distN = -big;
        // Undo the last increment to put us at the last pixel
        // (input is no longer needed)
        outPtr0 -= outInc0;  
        for (idx0 = max0; idx0 >= min0; --idx0)
          {
          if (*outPtr0 >= 0)
            {
            if (distP > *outPtr0)
              {
              distP = *outPtr0;
              }
            *outPtr0 = distP;
            }
          if (*outPtr0 <= 0)
            {
            if (distN < *outPtr0)
              {
              distN = *outPtr0;
              }
            *outPtr0 = distN;
            }
          
          if (distP < big)
            {
            ++distP;
            }
          if (distN > -big)
            {
            --distN;
            }
          
          outPtr0 -= outInc0;
          }
        
        inPtrC += 1;
        outPtrC += 1;
        }
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
    }     

  return 1;
}


