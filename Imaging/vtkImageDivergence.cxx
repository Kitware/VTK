/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDivergence.cxx
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
#include "vtkImageDivergence.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageDivergence, "1.24");
vtkStandardNewMacro(vtkImageDivergence);

//----------------------------------------------------------------------------
// This method tells the superclass that the first axis will collapse.
void vtkImageDivergence::ExecuteInformation(vtkImageData *vtkNotUsed(inData), 
                                            vtkImageData *outData)
{
  outData->SetNumberOfScalarComponents(1);
}

//----------------------------------------------------------------------------
// Just clip the request.  The subclass may need to overwrite this method.
void vtkImageDivergence::ComputeInputUpdateExtent(int inExt[6], 
                                                  int outExt[6])
{
  int idx;
  int *wholeExtent;
  int dimensionality = this->GetInput()->GetNumberOfScalarComponents();
  
  if (dimensionality > 3)
    {
    vtkErrorMacro("Divergence has to have dimensionality <= 3");
    dimensionality = 3;
    }
  
  // handle XYZ
  memcpy(inExt,outExt,sizeof(int)*6);
  
  wholeExtent = this->GetInput()->GetWholeExtent();
  // update and Clip
  for (idx = 0; idx < dimensionality; ++idx)
    {
    --inExt[idx*2];
    ++inExt[idx*2+1];
    if (inExt[idx*2] < wholeExtent[idx*2])
      {
      inExt[idx*2] = wholeExtent[idx*2];
      }
    if (inExt[idx*2] > wholeExtent[idx*2 + 1])
      {
      inExt[idx*2] = wholeExtent[idx*2 + 1];
      }
    if (inExt[idx*2+1] < wholeExtent[idx*2])
      {
      inExt[idx*2+1] = wholeExtent[idx*2];
      }
    if (inExt[idx*2 + 1] > wholeExtent[idx*2 + 1])
      {
      inExt[idx*2 + 1] = wholeExtent[idx*2 + 1];
      }
    }
}

//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
void vtkImageDivergenceExecute(vtkImageDivergence *self,
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
  int *wholeExtent, *inIncs;
  float r[3], d, sum;
  int useMin[3], useMax[3];
  
  // find the region to loop over
  maxC = inData->GetNumberOfScalarComponents();
  if (maxC > 3)
    {
    vtkGenericWarningMacro("Dimensionality must be less than or equal to 3");
    maxC = 3;
    }
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // The spacing is important for computing the gradient.
  // central differences (2 * ratio).
  // Negative because below we have (min - max) for dx ...
  inData->GetSpacing(r);
  r[0] = -0.5 / r[0];
  r[1] = -0.5 / r[1];
  r[2] = -0.5 / r[2];

  // get some other info we need
  inIncs = inData->GetIncrements(); 
  wholeExtent = inData->GetExtent(); 

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    useMin[2] = ((idxZ + outExt[4]) <= wholeExtent[4]) ? 0 : -inIncs[2];
    useMax[2] = ((idxZ + outExt[4]) >= wholeExtent[5]) ? 0 : inIncs[2];
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
      useMin[1] = ((idxY + outExt[2]) <= wholeExtent[2]) ? 0 : -inIncs[1];
      useMax[1] = ((idxY + outExt[2]) >= wholeExtent[3]) ? 0 : inIncs[1];
      for (idxX = 0; idxX <= maxX; idxX++)
        {
        useMin[0] = ((idxX + outExt[0]) <= wholeExtent[0]) ? 0 : -inIncs[0];
        useMax[0] = ((idxX + outExt[0]) >= wholeExtent[1]) ? 0 : inIncs[0];
        sum = 0.0;
        for (idxC = 0; idxC < maxC; idxC++)
          {
          // do X axis
          d = (float)(inPtr[useMin[idxC]]);
          d -= (float)(inPtr[useMax[idxC]]);
          sum += d * r[idxC];
          inPtr++;
          }
        *outPtr = (T)sum;
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
void vtkImageDivergence::ThreadedExecute(vtkImageData *inData, 
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
    vtkTemplateMacro7(vtkImageDivergenceExecute, this, inData, 
                      (VTK_TT *)(inPtr), outData, (VTK_TT *)(outPtr), 
                      outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

