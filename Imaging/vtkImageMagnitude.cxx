/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMagnitude.cxx
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
#include "vtkImageMagnitude.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageMagnitude, "1.31");
vtkStandardNewMacro(vtkImageMagnitude);

//----------------------------------------------------------------------------
// This method tells the superclass that the first axis will collapse.
void vtkImageMagnitude::ExecuteInformation(vtkImageData *vtkNotUsed(inData), 
                                           vtkImageData *outData)
{
  outData->SetNumberOfScalarComponents(1);
}

//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
static void vtkImageMagnitudeExecute(vtkImageMagnitude *self,
                                             vtkImageData *inData, T *inPtr,
                                             vtkImageData *outData, T *outPtr,
                                             int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  float sum;
  
  // find the region to loop over
  maxC = inData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
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
      for (idxX = 0; idxX <= maxX; idxX++)
        {
        sum = 0;
        for (idxC = 0; idxC < maxC; idxC++)
          {
          sum += (float) (*inPtr * *inPtr);
          inPtr++;
          }
        *outPtr = (T)(sqrt(sum));
        outPtr++;
        }
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}


//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The output data
// must match input type.  This method does handle boundary conditions.
void vtkImageMagnitude::ThreadedExecute(vtkImageData *inData, 
                                        vtkImageData *outData,
                                        int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  // This is really meta data and should be set in ExecuteInformation,
  // but there are some issues to solve first.
  if (id == 0 && outData->GetPointData()->GetScalars())
    {
    outData->GetPointData()->GetScalars()->SetName("Magnitude");
    }
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
    vtkTemplateMacro7(vtkImageMagnitudeExecute, this, inData, 
                      (VTK_TT *)(inPtr), outData, (VTK_TT *)(outPtr), 
                      outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}










