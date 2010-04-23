/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageExtractComponents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageExtractComponents.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageExtractComponents);

//----------------------------------------------------------------------------
vtkImageExtractComponents::vtkImageExtractComponents()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->Components[0] = 0;
  this->Components[1] = 1;
  this->Components[2] = 2;
  this->NumberOfComponents = 1;
}

//----------------------------------------------------------------------------
void vtkImageExtractComponents::SetComponents(int c1, int c2, int c3)
{
  int modified = 0;
  
  if (this->Components[0] != c1)
    {
    this->Components[0] = c1;
    modified = 1;
    }
  if (this->Components[1] != c2)
    {
    this->Components[1] = c2;
    modified = 1;
    }
  if (this->Components[2] != c3)
    {
    this->Components[2] = c3;
    modified = 1;
    }
  
  if (modified || this->NumberOfComponents != 3)
    {
    this->NumberOfComponents = 3;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageExtractComponents::SetComponents(int c1, int c2)
{
  int modified = 0;
  
  if (this->Components[0] != c1)
    {
    this->Components[0] = c1;
    modified = 1;
    }
  if (this->Components[1] != c2)
    {
    this->Components[1] = c2;
    modified = 1;
    }
  
  if (modified || this->NumberOfComponents != 2)
    {
    this->NumberOfComponents = 2;
    this->Modified();
    }
}
                                                        
//----------------------------------------------------------------------------
void vtkImageExtractComponents::SetComponents(int c1)
{
  int modified = 0;
  
  if (this->Components[0] != c1)
    {
    this->Components[0] = c1;
    modified = 1;
    }
  
  if (modified || this->NumberOfComponents != 1)
    {
    this->NumberOfComponents = 1;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// This method tells the superclass that only one component will remain.
int vtkImageExtractComponents::RequestInformation (
  vtkInformation       * vtkNotUsed( request ),
  vtkInformationVector ** vtkNotUsed( inputVector ), 
  vtkInformationVector * outputVector)
{
  vtkDataObject::SetPointDataActiveScalarInfo(
    outputVector->GetInformationObject(0), -1, this->NumberOfComponents);
  return 1;
}

//----------------------------------------------------------------------------
template <class T>
void vtkImageExtractComponentsExecute(vtkImageExtractComponents *self,
                                      vtkImageData *inData, T *inPtr,
                                      vtkImageData *outData, T *outPtr,
                                      int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxX, maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int cnt, inCnt;
  int offset1, offset2, offset3;
  unsigned long count = 0;
  unsigned long target;
  
  // find the region to loop over
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = static_cast<unsigned long>((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  cnt = outData->GetNumberOfScalarComponents();
  inCnt = inData->GetNumberOfScalarComponents();
  
  // Loop through output pixels
  offset1 = self->GetComponents()[0];
  offset2 = self->GetComponents()[1];
  offset3 = self->GetComponents()[2];
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!id) 
        {
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      // handle inner loop based on number of components extracted
      switch (cnt)
        {
        case 1:
          for (idxR = 0; idxR <= maxX; idxR++)
            {
            // Pixel operation
            *outPtr = *(inPtr + offset1);
            outPtr++;
            inPtr += inCnt;
            }
          break;
        case 2:
          for (idxR = 0; idxR <= maxX; idxR++)
            {
            // Pixel operation
            *outPtr = *(inPtr + offset1);
            outPtr++;
            *outPtr = *(inPtr + offset2);
            outPtr++;
            inPtr += inCnt;
            }
          break;
        case 3:
          for (idxR = 0; idxR <= maxX; idxR++)
            {
            // Pixel operation
            *outPtr = *(inPtr + offset1);
            outPtr++;
            *outPtr = *(inPtr + offset2);
            outPtr++;
            *outPtr = *(inPtr + offset3);
            outPtr++;
            inPtr += inCnt;
            }
          break;
        }
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}


//----------------------------------------------------------------------------
// This method is passed input and output datas, and executes the
// ExtractComponents function on each line.  
void vtkImageExtractComponents::ThreadedExecute (vtkImageData *inData, 
                                                vtkImageData *outData,
                                                int outExt[6], int id)
{
  int max, idx;
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " 
                  << inData->GetScalarType()
                  << ", must match out ScalarType " 
                  << outData->GetScalarType());
    return;
    }
  
  // make sure we can get all of the components.
  max = inData->GetNumberOfScalarComponents();
  for (idx = 0; idx < this->NumberOfComponents; ++idx)
    {
    if (this->Components[idx] >= max || this->Components[idx] < 0)
      {
      vtkErrorMacro("Execute: Component " << this->Components[idx]
                    << " is not in input.");
      return;
      }
    }
  
  // choose which templated function to call.
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageExtractComponentsExecute(this, inData, 
                                       static_cast<VTK_TT *>(inPtr), outData, 
                                       static_cast<VTK_TT *>(outPtr),
                                       outExt, id));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

void vtkImageExtractComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  os << indent << "Components: ( "
     << this->Components[0] << ", "
     << this->Components[1] << ", "
     << this->Components[2] << " )\n";

}

