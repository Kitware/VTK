/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLogarithmicScale.cxx
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
#include "vtkImageLogarithmicScale.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageLogarithmicScale, "1.19");
vtkStandardNewMacro(vtkImageLogarithmicScale);

//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageLogarithmicScale::vtkImageLogarithmicScale()
{
  this->Constant = 10.0;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageLogarithmicScaleExecute(vtkImageLogarithmicScale *self,
                                            vtkImageData *inData, T *inPtr,
                                            vtkImageData *outData, T *outPtr, 
                                            int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  float c;

  c = self->GetConstant();

  // find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1)*inData->GetNumberOfScalarComponents();
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through ouput pixels
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
      for (idxR = 0; idxR < rowLength; idxR++)
        {

        // Pixel operation
        if (*inPtr > 0)
          {
          *outPtr = (T)(c*log((double)(*inPtr)+1.0));
          }
        else
          {
          *outPtr = (T)(-c*log(1.0-(double)(*inPtr)));
          }

        outPtr++;
        inPtr++;
        }
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}


//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageLogarithmicScale::ThreadedExecute(vtkImageData *inData, 
                                        vtkImageData *outData,
                                        int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
    << ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
                << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro7(vtkImageLogarithmicScaleExecute, this, inData, 
                      (VTK_TT *)(inPtr), outData, (VTK_TT *)(outPtr),
                      outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

void vtkImageLogarithmicScale::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Constant: " << this->Constant << "\n";
}

