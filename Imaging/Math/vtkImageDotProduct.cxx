/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDotProduct.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageDotProduct.h"

#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageDotProduct);

//----------------------------------------------------------------------------
vtkImageDotProduct::vtkImageDotProduct()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
// Colapse the first axis
int vtkImageDotProduct::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  vtkDataObject::SetPointDataActiveScalarInfo(
    outputVector->GetInformationObject(0), -1, 1);
  return 1;
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
        dot += static_cast<float>(*inSI1 * *inSI2);
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
void vtkImageDotProduct::ThreadedRequestData(
  vtkInformation * vtkNotUsed( request ),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector * vtkNotUsed( outputVector ),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input1 ScalarType, "
                  <<  inData[0][0]->GetScalarType()
                  << ", must match output ScalarType "
                  << outData[0]->GetScalarType());
    return;
    }

  if (inData[1][0]->GetScalarType() != outData[0]->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input2 ScalarType, "
                  << inData[1][0]->GetScalarType()
                  << ", must match output ScalarType "
                  << outData[0]->GetScalarType());
    return;
    }

  // this filter expects that inputs that have the same number of components
  if (inData[0][0]->GetNumberOfScalarComponents() !=
      inData[1][0]->GetNumberOfScalarComponents())
    {
    vtkErrorMacro(<< "Execute: input1 NumberOfScalarComponents, "
                  << inData[0][0]->GetNumberOfScalarComponents()
                  << ", must match out input2 NumberOfScalarComponents "
                  << inData[1][0]->GetNumberOfScalarComponents());
    return;
    }

  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageDotProductExecute(this, inData[0][0],
                                inData[1][0], outData[0], outExt, id,
                                static_cast<VTK_TT *>(0)));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}














