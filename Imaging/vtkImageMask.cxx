/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMask.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageMask.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkImageMask, "1.28");
vtkStandardNewMacro(vtkImageMask);

//----------------------------------------------------------------------------
vtkImageMask::vtkImageMask()
{
  this->NotMask = 0;
  this->MaskedOutputValue = new float[3];
  this->MaskedOutputValueLength = 3;
  this->MaskedOutputValue[0] = this->MaskedOutputValue[1] 
    = this->MaskedOutputValue[2] = 0.0;
}

vtkImageMask::~vtkImageMask()
{
  delete [] this->MaskedOutputValue;
}


//----------------------------------------------------------------------------
void vtkImageMask::SetMaskedOutputValue(int num, float *v)
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
    this->MaskedOutputValue = new float[num];
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
static void vtkImageMaskExecute(vtkImageMask *self, int ext[6],
                                vtkImageData *in1Data, T *in1Ptr,
                                vtkImageData *in2Data, unsigned char *in2Ptr,
                                vtkImageData *outData, T *outPtr, int id)
{
  int num0, num1, num2, numC, pixSize;
  int idx0, idx1, idx2;
  int in1Inc0, in1Inc1, in1Inc2;
  int in2Inc0, in2Inc1, in2Inc2;
  int outInc0, outInc1, outInc2;
  T *maskedValue;
  float *v;
  int nv;
  int maskState;
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
    maskedValue[idx0] = (T)(v[idx1]);
    }
  pixSize = numC * sizeof(T);
  maskState = self->GetNotMask();
  
  // Get information to march through data 
  in1Data->GetContinuousIncrements(ext, in1Inc0, in1Inc1, in1Inc2);
  in2Data->GetContinuousIncrements(ext, in2Inc0, in2Inc1, in2Inc2);
  outData->GetContinuousIncrements(ext, outInc0, outInc1, outInc2);
  num0 = ext[1] - ext[0] + 1;
  num1 = ext[3] - ext[2] + 1;
  num2 = ext[5] - ext[4] + 1;
  
  target = (unsigned long)(num2*num1/50.0);
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
void vtkImageMask::ThreadedExecute(vtkImageData **inData, 
                                   vtkImageData *outData,
                                   int outExt[6], int id)
{
  void *inPtr1;
  void *inPtr2;
  void *outPtr;
  int *tExt;
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
                << ", outData = " << outData);
  

  if (inData[0] == NULL)
    {
    vtkErrorMacro(<< "Input " << 0 << " must be specified.");
    return;
    }
  if (inData[1] == NULL)
    {
    vtkErrorMacro(<< "Input " << 1 << " must be specified.");
    return;
    }

inPtr1 = inData[0]->GetScalarPointerForExtent(outExt);
  inPtr2 = inData[1]->GetScalarPointerForExtent(outExt);
  outPtr = outData->GetScalarPointerForExtent(outExt);

  tExt = inData[1]->GetExtent();
  if (tExt[0] > outExt[0] || tExt[1] < outExt[1] || 
      tExt[2] > outExt[2] || tExt[3] < outExt[3] ||
      tExt[4] > outExt[4] || tExt[5] < outExt[5])
    {
    vtkErrorMacro("Mask extent not large enough");
    return;
    }
  
  if (inData[1]->GetNumberOfScalarComponents() != 1)
    {
    vtkErrorMacro("Maks can have one comenent");
    }
    
  if (inData[0]->GetScalarType() != outData->GetScalarType() ||
      inData[1]->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro(<< "Execute: image ScalarType (" 
      << inData[0]->GetScalarType() << ") must match out ScalarType (" 
      << outData->GetScalarType() << "), and mask scalar type (" 
      << inData[1]->GetScalarType() << ") must be unsigned char.");
    return;
    }
  
  switch (inData[0]->GetScalarType())
    {
    vtkTemplateMacro9(vtkImageMaskExecute, this, outExt, inData[0], 
                      (VTK_TT *)(inPtr1), inData[1], (unsigned char *)(inPtr2),
                      outData, (VTK_TT *)(outPtr),id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
// The output extent is the intersection.
void vtkImageMask::ExecuteInformation(vtkImageData **inDatas, 
                                      vtkImageData *outData)
{
  int ext[6], *ext2, idx;

  if (inDatas == NULL || inDatas[0] == NULL || inDatas[1] == NULL)
    {
    vtkErrorMacro("Missing and input.");
    return;
    }
  
  inDatas[0]->GetWholeExtent(ext);
  ext2 = this->GetInput(1)->GetWholeExtent();
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
  
  outData->SetWholeExtent(ext);
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
}

