/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFlip.cxx
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
#include "vtkImageFlip.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkImageFlip, "1.31");
vtkStandardNewMacro(vtkImageFlip);

//----------------------------------------------------------------------------
vtkImageFlip::vtkImageFlip()
{
  this->PreserveImageExtent = 1;
  this->FilteredAxis = 0;
}

//----------------------------------------------------------------------------
// Image extent is modified by this filter.
void vtkImageFlip::ExecuteInformation(vtkImageData *inData, 
                                      vtkImageData *outData)
{
  int extent[6];
  int axis, temp;

  if ( ! this->PreserveImageExtent)
    {
    inData->GetWholeExtent(extent);
    axis = this->FilteredAxis;
    temp = extent[axis*2+1];
    extent[axis*2+1] = -temp;
    outData->SetWholeExtent(extent);
    }
}

//----------------------------------------------------------------------------
// What input should be requested.
void vtkImageFlip::ComputeInputUpdateExtent(int inExt[6], 
                                            int outExt[6])
{
  int axis, sum;
  int *wholeExtent;
  
  // copy out to in
  memcpy((void *)inExt, (void *)outExt, 6 * sizeof(int));
  
  wholeExtent = this->GetOutput()->GetWholeExtent();
  axis = this->FilteredAxis;
  if (this->PreserveImageExtent)
    {
    sum = wholeExtent[axis*2] + wholeExtent[axis*2+1];
    inExt[axis*2] = -outExt[axis*2+1]+sum;
    inExt[axis*2+1] = -outExt[axis*2]+sum;
    }
  else
    {
    inExt[axis*2] = -outExt[axis*2+1];
    inExt[axis*2+1] = -outExt[axis*2];
    }
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageFlipExecute(vtkImageFlip *self, int id,
                         vtkImageData *inData, int *inExt,
                         vtkImageData *outData, int *outExt, T *outPtr)
{
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outSkipY, outSkipZ;
  T *inPtrX, *inPtrY, *inPtrZ;
  int scalarSize;
  unsigned long count = 0;
  unsigned long target;

  // find the region to loop over
  maxX = outExt[1] - outExt[0]; 
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  // target is for progress ...
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through data 
  inData->GetIncrements(inIncX, inIncY, inIncZ);
  // outIncX is a place holder
  outData->GetContinuousIncrements(outExt, outIncX, outSkipY, outSkipZ);
  // for x, increment is used because all components are copied at once
  outIncX = inData->GetNumberOfScalarComponents();
  // for memcpy
  scalarSize = sizeof(T)*outIncX;
  
  // Get the starting in pointer
  inPtrZ = (T *)inData->GetScalarPointerForExtent(inExt);
  // adjust the increments (and pointer) for the flip
  switch (self->GetFilteredAxis())
    {
    case 0:
      inPtrZ += maxX * inIncX;
      inIncX = -inIncX;
      break;
    case 1:
      inPtrZ += maxY * inIncY;
      inIncY = -inIncY;
      break;
    case 2:
      inPtrZ += maxZ * inIncZ;
      inIncZ = -inIncZ;
      break;
    default:
      vtkGenericWarningMacro("Bad axis " << self->GetFilteredAxis());
      return;
    }
  
  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    inPtrY = inPtrZ;
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
      {
      // handle updating progress method
      if (!id) 
        {
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      inPtrX = inPtrY;
      for (idxX = 0; idxX <= maxX; idxX++)
        {
        // Pixel operation
        memcpy((void *)outPtr, (void *)inPtrX, scalarSize);
        outPtr += outIncX;
        inPtrX += inIncX;
        }
      outPtr += outSkipY;
      inPtrY += inIncY;
      }
    outPtr += outSkipZ;
    inPtrZ += inIncZ;
    }
}



//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageFlip::ThreadedExecute(vtkImageData *inData, 
                                   vtkImageData *outData,
                                   int outExt[6], int id) 
{
  int inExt[6];
  void *outPtr;
  
  outPtr = outData->GetScalarPointerForExtent(outExt);
  this->ComputeInputUpdateExtent(inExt, outExt);

  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
               << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  switch (outData->GetScalarType())
    {
    vtkTemplateMacro7(vtkImageFlipExecute, this, id, inData, inExt, 
                      outData, outExt, (VTK_TT *)(outPtr));
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

void vtkImageFlip::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FilteredAxis: " << this->FilteredAxis << "\n";

  os << indent << "PreserveImageExtent: " << (this->PreserveImageExtent ? "On\n" : "Off\n");


}

