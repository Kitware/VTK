/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMagnitude.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageMagnitude.h"

#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageMagnitude, "1.36.10.4");
vtkStandardNewMacro(vtkImageMagnitude);

//----------------------------------------------------------------------------
vtkImageMagnitude::vtkImageMagnitude()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

void vtkImageMagnitude::ExecuteInformation(
  vtkInformation       * vtkNotUsed( request ),
  vtkInformationVector * vtkNotUsed( inputVector ), 
  vtkInformationVector * outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS(),1);
}

//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
void vtkImageMagnitudeExecute(vtkImageMagnitude *self,
                              vtkImageData *inData,
                              vtkImageData *outData,
                              int outExt[6], int id, T *)
{
  vtkImageIterator<T> inIt(inData, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  float sum;

  // find the region to loop over
  int maxC = inData->GetNumberOfScalarComponents();
  int idxC;

  // Loop through ouput pixels
  while (!outIt.IsAtEnd())
    {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
      {
      // now process the components
      sum = 0.0;
      for (idxC = 0; idxC < maxC; idxC++)
        {
        sum += static_cast<float>(*inSI * *inSI);
        ++inSI;
        }
      *outSI = static_cast<T>(sqrt(sum));
      ++outSI;
      }
    inIt.NextSpan();
    outIt.NextSpan();
    }
}


//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The output data
// must match input type.  This method does handle boundary conditions.
void vtkImageMagnitude::ThreadedExecute (vtkImageData ***inData, 
                                        vtkImageData **outData,
                                        int outExt[6], int id)
{
  // This is really meta data and should be set in ExecuteInformation,
  // but there are some issues to solve first.
  if (id == 0 && outData[0]->GetPointData()->GetScalars())
    {
    outData[0]->GetPointData()->GetScalars()->SetName("Magnitude");
    }
  vtkDebugMacro(<< "Execute: inData = " << inData 
  << ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData[0][0]->GetScalarType()
    << ", must match out ScalarType " << outData[0]->GetScalarType());
    return;
    }
  
  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro6(vtkImageMagnitudeExecute, this, inData[0][0], outData[0], 
                      outExt, id, static_cast<VTK_TT *>(0));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}










