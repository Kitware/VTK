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
#include "vtkImageProgressIterator.h"

vtkCxxRevisionMacro(vtkImageAppendComponents, "1.23");
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
void vtkImageAppendComponentsExecute(vtkImageAppendComponents *self,
                                     vtkImageData *inData, 
                                     vtkImageData *outData, 
                                     int outComp,
                                            int outExt[6], int id, T *)
{
  vtkImageIterator<T> inIt(inData, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  int numIn = inData->GetNumberOfScalarComponents();
  int numSkip = outData->GetNumberOfScalarComponents() - numIn;
  int i;
  
  // Loop through ouput pixels
  while (!outIt.IsAtEnd())
    {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan() + outComp;
    T* outSIEnd = outIt.EndSpan();
    while (outSI < outSIEnd)
      {
      // now process the components
      for (i = 0; i < numIn; ++i)
        {
        *outSI = *inSI;
        ++outSI;
        ++inSI;
        }
      outSI = outSI + numSkip;
      }
    inIt.NextSpan();
    outIt.NextSpan();
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
  int idx1, outComp;

  outComp = 0;
  for (idx1 = 0; idx1 < this->NumberOfInputs; ++idx1)
    {
    if (inData[idx1] != NULL)
      {
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
        vtkTemplateMacro7(vtkImageAppendComponentsExecute, this, 
                          inData[idx1], outData, 
                          outComp, outExt, id, static_cast<VTK_TT *>(0));
       default:
         vtkErrorMacro(<< "Execute: Unknown ScalarType");
         return;
       }
      outComp += inData[idx1]->GetNumberOfScalarComponents();
      }
    }
}
















