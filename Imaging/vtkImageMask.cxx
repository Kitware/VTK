/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMask.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageMask.h"

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageMask);

//----------------------------------------------------------------------------
vtkImageMask::vtkImageMask()
{
  this->NotMask = 0;
  this->MaskedOutputValue = new double[3];
  this->MaskedOutputValueLength = 3;
  this->MaskedOutputValue[0] = this->MaskedOutputValue[1]
    = this->MaskedOutputValue[2] = 0.0;
  this->MaskAlpha = 1.0;
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkImageMask::~vtkImageMask()
{
  delete [] this->MaskedOutputValue;
}

//----------------------------------------------------------------------------
void vtkImageMask::SetImageInput(vtkImageData *in)
{
  this->SetInput1(in);
}

//----------------------------------------------------------------------------
void vtkImageMask::SetMaskInput(vtkImageData *in)
{
  this->SetInput2(in);
}

//----------------------------------------------------------------------------
void vtkImageMask::SetMaskedOutputValue(int num, double *v)
{
  int idx;

  if (num < 1)
    {
    vtkErrorMacro("Output value must have length greater than 0");
    return;
    }
  if (num != this->MaskedOutputValueLength)
    {
    this->Modified();
    }
  
  if (num > this->MaskedOutputValueLength)
    {
    delete [] this->MaskedOutputValue;
    this->MaskedOutputValue = new double[num];
    this->MaskedOutputValueLength = num;
    }

  this->MaskedOutputValueLength = num;
  for (idx = 0; idx < num; ++ idx)
    {
    if (this->MaskedOutputValue[idx] != v[idx])
      {
      this->Modified();
      }
    this->MaskedOutputValue[idx] = v[idx];
    }
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageMaskExecute(vtkImageMask *self, int ext[6],
                         vtkImageData *in1Data, T *in1Ptr,
                         vtkImageData *in2Data, unsigned char *in2Ptr,
                         vtkImageData *outData, T *outPtr, int id)
{
  int num0, num1, num2, numC, pixSize;
  int idx0, idx1, idx2, idxC;
  vtkIdType in1Inc0, in1Inc1, in1Inc2;
  vtkIdType in2Inc0, in2Inc1, in2Inc2;
  vtkIdType outInc0, outInc1, outInc2;
  T *maskedValue;
  double *v;
  int nv;
  int maskState;
  double maskAlpha, oneMinusMaskAlpha;
  unsigned long count = 0;
  unsigned long target;

  // create a masked output value with the correct length by cycling
  numC = outData->GetNumberOfScalarComponents();
  maskedValue = new T[numC];
  v = self->GetMaskedOutputValue();
  nv = self->GetMaskedOutputValueLength();
  for (idx0 = 0, idx1 = 0; idx0 < numC; ++idx0, ++idx1)
    {
    if (idx1 >= nv)
      {
      idx1 = 0;
      }
    maskedValue[idx0] = static_cast<T>(v[idx1]);
    }
  pixSize = numC * sizeof(T);
  maskState = self->GetNotMask();
  maskAlpha = self->GetMaskAlpha();
  oneMinusMaskAlpha = 1.0 - maskAlpha;

  // Get information to march through data
  in1Data->GetContinuousIncrements(ext, in1Inc0, in1Inc1, in1Inc2);
  in2Data->GetContinuousIncrements(ext, in2Inc0, in2Inc1, in2Inc2);
  outData->GetContinuousIncrements(ext, outInc0, outInc1, outInc2);
  num0 = ext[1] - ext[0] + 1;
  num1 = ext[3] - ext[2] + 1;
  num2 = ext[5] - ext[4] + 1;

  target = static_cast<unsigned long>(num2*num1/50.0);
  target++;

  // Loop through ouput pixels
  for (idx2 = 0; idx2 < num2; ++idx2)
    {
    for (idx1 = 0; !self->AbortExecute && idx1 < num1; ++idx1)
      {
      if (!id)
        {
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      for (idx0 = 0; idx0 < num0; ++idx0)
        {
        if ( maskAlpha == 1.0 )
          {
          // Pixel operation
          if (*in2Ptr && maskState == 1)
            {
            memcpy(outPtr, maskedValue, pixSize);
            }
          else if ( ! *in2Ptr && maskState == 0)
            {
            memcpy(outPtr, maskedValue, pixSize);
            }
          else
            {
            memcpy(outPtr, in1Ptr, pixSize);
            }
          in1Ptr += numC;
          outPtr += numC;
          }
        else
          {
          // We need to do an over operation
          int doMask = 0;
          if ( *in2Ptr && maskState == 1 )
            {
            doMask = 1;
            }
          else if ( !*in2Ptr && maskState == 0 )
            {
            doMask = 1;
            }
          if ( doMask )
            {
            // Do an over operation
            for ( idxC = 0; idxC < numC; ++idxC )
              {
              *outPtr = static_cast<T>(oneMinusMaskAlpha * *in1Ptr + maskedValue[idxC] * maskAlpha );
              ++outPtr;
              ++in1Ptr;
              }
            }
          else
            {
            // Copy verbatum
            for ( idxC = 0; idxC < numC; ++idxC )
              {
              *outPtr = *in1Ptr;
              ++outPtr;
              ++in1Ptr;
              }
            }
          }
        in2Ptr += 1;
        }
      in1Ptr += in1Inc1;
      in2Ptr += in2Inc1;
      outPtr += outInc1;
      }
    in1Ptr += in1Inc2;
    in2Ptr += in2Inc2;
    outPtr += outInc2;
    }
  
  delete [] maskedValue;
}



//----------------------------------------------------------------------------
// This method is passed a input and output Datas, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the Datas data types.
void vtkImageMask::ThreadedRequestData(
  vtkInformation * vtkNotUsed( request ), 
  vtkInformationVector ** vtkNotUsed( inputVector ), 
  vtkInformationVector * vtkNotUsed( outputVector ),
  vtkImageData ***inData, 
  vtkImageData **outData,
  int outExt[6], int id)
{
  void *inPtr1;
  void *inPtr2;
  void *outPtr;
  int *tExt;
  
  inPtr1 = inData[0][0]->GetScalarPointerForExtent(outExt);
  inPtr2 = inData[1][0]->GetScalarPointerForExtent(outExt);
  outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  tExt = inData[1][0]->GetExtent();
  if (tExt[0] > outExt[0] || tExt[1] < outExt[1] || 
      tExt[2] > outExt[2] || tExt[3] < outExt[3] ||
      tExt[4] > outExt[4] || tExt[5] < outExt[5])
    {
    vtkErrorMacro("Mask extent not large enough");
    return;
    }

  if (inData[1][0]->GetNumberOfScalarComponents() != 1)
    {
    vtkErrorMacro("Masks can have one component");
    }

  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType() ||
      inData[1][0]->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro(<< "Execute: image ScalarType ("
      << inData[0][0]->GetScalarType() << ") must match out ScalarType ("
      << outData[0]->GetScalarType() << "), and mask scalar type ("
      << inData[1][0]->GetScalarType() << ") must be unsigned char.");
    return;
    }

  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageMaskExecute(this, outExt, inData[0][0], 
                          static_cast<VTK_TT *>(inPtr1), inData[1][0], 
                          static_cast<unsigned char *>(inPtr2),
                          outData[0], static_cast<VTK_TT *>(outPtr),id));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
// The output extent is the intersection.
int vtkImageMask::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *inInfo2 = inputVector[1]->GetInformationObject(0);

  int ext[6], ext2[6], idx;

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext);
  inInfo2->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext2);
  for (idx = 0; idx < 3; ++idx)
    {
    if (ext2[idx*2] > ext[idx*2])
      {
      ext[idx*2] = ext2[idx*2];
      }
    if (ext2[idx*2+1] < ext[idx*2+1])
      {
      ext[idx*2+1] = ext2[idx*2+1];
      }
    }
  
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext,6);

  return 1;
}

void vtkImageMask::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  int idx;

  os << indent << "MaskedOutputValue: " << this->MaskedOutputValue[0];
  for (idx = 1; idx < this->MaskedOutputValueLength; ++idx)
    {
    os << ", " << this->MaskedOutputValue[idx];
    }
  os << endl;

  os << indent << "NotMask: " << (this->NotMask ? "On\n" : "Off\n");
  os << indent << "MaskAlpha: " << this->MaskAlpha << "\n";
}

