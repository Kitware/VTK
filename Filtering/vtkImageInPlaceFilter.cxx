/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageInPlaceFilter.cxx
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
#include "vtkImageInPlaceFilter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkImageInPlaceFilter, "1.34");
vtkStandardNewMacro(vtkImageInPlaceFilter);

//----------------------------------------------------------------------------

void vtkImageInPlaceFilter::ExecuteData(vtkDataObject *vtkNotUsed(out))
{
  vtkImageData *output = this->GetOutput();
  int *inExt, *outExt;
  
  inExt = this->GetInput()->GetUpdateExtent();
  outExt = this->GetOutput()->GetUpdateExtent();

  vtkImageData *input;
  input = this->GetInput();

  if ((inExt[0] == outExt[0])&&(inExt[1] == outExt[1])&&
      (inExt[2] == outExt[2])&&(inExt[3] == outExt[3])&&
      (inExt[4] == outExt[4])&&(inExt[5] == outExt[5])&&
      this->GetInput()->ShouldIReleaseData())
    {
    // pass the data
    output->GetPointData()->PassData(input->GetPointData());
    output->SetExtent(this->GetInput()->GetExtent());
    }
  else
    {
    output->SetExtent(output->GetUpdateExtent());
    output->AllocateScalars();
    this->CopyData(input,output);
    }
}
  
  

void vtkImageInPlaceFilter::CopyData(vtkImageData *inData,
                                     vtkImageData *outData)
{
  int *outExt = this->GetOutput()->GetUpdateExtent();
  char *inPtr = (char *) inData->GetScalarPointerForExtent(outExt);
  char *outPtr = (char *) outData->GetScalarPointerForExtent(outExt);
  int rowLength, size;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int idxY, idxZ, maxY, maxZ;
  
  rowLength = (outExt[1] - outExt[0]+1)*inData->GetNumberOfScalarComponents();
  size = inData->GetScalarSize();
  rowLength *= size;
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  
  // adjust increments for this loop
  inIncY = inIncY*size + rowLength;
  outIncY = outIncY*size + rowLength;
  inIncZ *= size;
  outIncZ *= size;
  
  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      memcpy(outPtr,inPtr,rowLength);
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}
