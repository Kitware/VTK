/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNormalize.cxx
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
#include "vtkImageNormalize.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageNormalize, "1.9");
vtkStandardNewMacro(vtkImageNormalize);

//----------------------------------------------------------------------------
// This method tells the superclass that the first axis will collapse.
void vtkImageNormalize::ExecuteInformation(vtkImageData *vtkNotUsed(inData), 
                                           vtkImageData *outData)
{
  outData->SetScalarType(VTK_FLOAT);
}

//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
static void vtkImageNormalizeExecute(vtkImageNormalize *self,
                                     vtkImageData *inData, T *inPtr,
                                     vtkImageData *outData, float *outPtr,
                                     int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  float sum;
  T *inVect;
  
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

        // save the start of the vector
        inVect = inPtr;

        // compute the magnitude.
        sum = 0.0;
        for (idxC = 0; idxC < maxC; idxC++)
          {
          sum += (float)(*inPtr) * (float)(*inPtr);
          inPtr++;
          }
        if (sum > 0.0)
          {
          sum = 1.0 / sqrt(sum);
          }
        
        // now divide to normalize.
        for (idxC = 0; idxC < maxC; idxC++)
          {
          *outPtr = (float)(*inVect) * sum;
          inVect++;
          outPtr++;
          }

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
void vtkImageNormalize::ThreadedExecute(vtkImageData *inData, 
                                        vtkImageData *outData,
                                        int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
  << ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (outData->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: output ScalarType, " << outData->GetScalarType()
    << ", must be float");
    return;
    }
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro7(vtkImageNormalizeExecute, this, inData,(VTK_TT *)(inPtr),
                     outData, (float *)(outPtr), outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}












