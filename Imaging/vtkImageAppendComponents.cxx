/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAppendComponents.cxx
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
#include "vtkImageAppendComponents.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkImageAppendComponents, "1.21");
vtkStandardNewMacro(vtkImageAppendComponents);

//----------------------------------------------------------------------------
// This method tells the ouput it will have more components
void vtkImageAppendComponents::ExecuteInformation(vtkImageData **inputs, 
                                                  vtkImageData *output)
{
  int idx1, num;

  num = 0;
  for (idx1 = 0; idx1 < this->NumberOfInputs; ++idx1)
    {
    if (inputs[idx1] != NULL)
      {
      num += inputs[idx1]->GetNumberOfScalarComponents();
      }
    }
  output->SetNumberOfScalarComponents(num);
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageAppendComponentsExecute(vtkImageAppendComponents *self,
                                            vtkImageData *inData, 
                                            T *inPtrP, int inComp,
                                            vtkImageData *outData, 
                                            T *outPtrP, int outComp,
                                            int outExt[6], int id)
{
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  T *outPtr, *inPtr;
  
  // find the region to loop over
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)(outData->GetNumberOfScalarComponents()*
                           (maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  inIncX = inData->GetNumberOfScalarComponents();
  outIncX = outData->GetNumberOfScalarComponents();
  

  outPtr = outPtrP + outComp;
  inPtr = inPtrP + inComp;
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!id) 
        {
        if (!(count%target)) 
          {
          self->UpdateProgress(count/(50.0*target) 
                               + (maxZ+1)*(maxY+1)*outComp);
          }
        count++;
        }
      for (idxX = 0; idxX <= maxX; idxX++)
        {
        // Pixel operation
        *outPtr = *inPtr;
        outPtr += outIncX;
        inPtr += inIncX;
        }
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}

//----------------------------------------------------------------------------
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageAppendComponents::ThreadedExecute(vtkImageData **inData, 
                                               vtkImageData *outData,
                                               int outExt[6], int id)
{
  int idx1, inComp, outComp, num;
  void *inPtr;
  void *outPtr = outData->GetScalarPointerForExtent(outExt);

  outComp = -1;
  for (idx1 = 0; idx1 < this->NumberOfInputs; ++idx1)
    {
    if (inData[idx1] != NULL)
      {
      inPtr = inData[idx1]->GetScalarPointerForExtent(outExt);
      num = inData[idx1]->GetNumberOfScalarComponents();
      // inefficient to have this loop here (could be inner loop)
      for (inComp = 0; inComp < num; ++inComp)
        {
        ++outComp;
        // this filter expects that input is the same type as output.
        if (inData[idx1]->GetScalarType() != outData->GetScalarType())
          {
          vtkErrorMacro(<< "Execute: input" << idx1 << " ScalarType (" << 
          inData[idx1]->GetScalarType() << 
          "), must match output ScalarType (" << outData->GetScalarType() 
          << ")");
          return;
          }
        
        switch (inData[idx1]->GetScalarType())
          {
          vtkTemplateMacro9(vtkImageAppendComponentsExecute, this, 
                            inData[idx1], (VTK_TT *)(inPtr), inComp,
                            outData, (VTK_TT *)(outPtr), 
                            outComp, outExt, id);
          default:
            vtkErrorMacro(<< "Execute: Unknown ScalarType");
            return;
          }
        }
      }
    }
  
}
















