/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDotProduct.cxx
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
#include "vtkImageDotProduct.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkImageDotProduct, "1.23");
vtkStandardNewMacro(vtkImageDotProduct);

//----------------------------------------------------------------------------
// Colapse the first axis
void vtkImageDotProduct::ExecuteInformation(vtkImageData **vtkNotUsed(inDatas),
                                            vtkImageData *outData)
{
  outData->SetNumberOfScalarComponents(1);
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
static void vtkImageDotProductExecute(vtkImageDotProduct *self,
                                      vtkImageData *in1Data, 
                                      T *in1Ptr,
                                      vtkImageData *in2Data, 
                                      T *in2Ptr,
                                      vtkImageData *outData, 
                                      T *outPtr,
                                      int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int in2IncX, in2IncY, in2IncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  float dot;
  
  // find the region to loop over
  maxC = in1Data->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  in2Data->GetContinuousIncrements(outExt, in2IncX, in2IncY, in2IncZ);
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
        // now process the components
        dot = 0.0;
        for (idxC = 0; idxC < maxC; idxC++)
          {
          // Pixel operation
          dot += (float)(*in1Ptr * *in2Ptr);
          in1Ptr++;
          in2Ptr++;
          }
        *outPtr = (T)dot;
        outPtr++;
        }
      outPtr += outIncY;
      in1Ptr += inIncY;
      in2Ptr += in2IncY;
      }
    outPtr += outIncZ;
    in1Ptr += inIncZ;
    in2Ptr += in2IncZ;
    }
}


//----------------------------------------------------------------------------
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageDotProduct::ThreadedExecute(vtkImageData **inData, 
                                         vtkImageData *outData,
                                         int outExt[6], int id)
{
  void *in1Ptr;
  void *in2Ptr;
  void *outPtr;
  
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
  in1Ptr = inData[0]->GetScalarPointerForExtent(outExt);
  in2Ptr = inData[1]->GetScalarPointerForExtent(outExt);
  outPtr = outData->GetScalarPointerForExtent(outExt);
  
  // this filter expects that input is the same type as output.
  if (inData[0]->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input1 ScalarType, "
                  <<  inData[0]->GetScalarType()
                  << ", must match output ScalarType "
                  << outData->GetScalarType());
    return;
    }
  
  if (inData[1]->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input2 ScalarType, "
                  << inData[1]->GetScalarType()
                  << ", must match output ScalarType "
                  << outData->GetScalarType());
    return;
    }
  
  // this filter expects that inputs that have the same number of components
  if (inData[0]->GetNumberOfScalarComponents() != 
      inData[1]->GetNumberOfScalarComponents())
    {
    vtkErrorMacro(<< "Execute: input1 NumberOfScalarComponents, "
                  << inData[0]->GetNumberOfScalarComponents()
                  << ", must match out input2 NumberOfScalarComponents "
                  << inData[1]->GetNumberOfScalarComponents());
    return;
    }

  switch (inData[0]->GetScalarType())
    {
    vtkTemplateMacro9(vtkImageDotProductExecute, this, inData[0], 
                      (VTK_TT *)(in1Ptr), inData[1], (VTK_TT *)(in2Ptr), 
                      outData, (VTK_TT *)(outPtr), outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}














