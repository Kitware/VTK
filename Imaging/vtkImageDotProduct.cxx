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
#include "vtkImageProgressIterator.h"

vtkCxxRevisionMacro(vtkImageDotProduct, "1.26");
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
void vtkImageDotProductExecute(vtkImageDotProduct *self,
                               vtkImageData *in1Data, 
                               vtkImageData *in2Data, 
                               vtkImageData *outData, 
                               int outExt[6], int id, T *)
{
  vtkImageIterator<T> inIt1(in1Data, outExt);
  vtkImageIterator<T> inIt2(in2Data, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  float dot;
  
  // find the region to loop over
  int maxC = in1Data->GetNumberOfScalarComponents();
  int idxC;
  
  // Loop through ouput pixels
  while (!outIt.IsAtEnd())
    {
    T* inSI1 = inIt1.BeginSpan();
    T* inSI2 = inIt2.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
      {
      // now process the components
      dot = 0.0;
      for (idxC = 0; idxC < maxC; idxC++)
        {
        dot += (float)(*inSI1 * *inSI2);
        ++inSI1;
        ++inSI2;
        }
      *outSI = static_cast<T>(dot);
      ++outSI;
      }
    inIt1.NextSpan();
    inIt2.NextSpan();
    outIt.NextSpan();
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
    vtkTemplateMacro7(vtkImageDotProductExecute, this, inData[0], 
                      inData[1], outData, outExt, id, 
                      static_cast<VTK_TT *>(0));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}














