// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageLuminance.h"

#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageLuminance);

//------------------------------------------------------------------------------
void vtkImageLuminance::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkImageLuminance::vtkImageLuminance()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
// This method overrides information set by parent's ExecuteInformation.
int vtkImageLuminance::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkDataObject::SetPointDataActiveScalarInfo(outputVector->GetInformationObject(0), -1, 1);
  return 1;
}

//------------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values
// out of extent.
template <class T>
void vtkImageLuminanceExecute(
  vtkImageLuminance* self, vtkImageData* inData, vtkImageData* outData, int outExt[6], int id, T*)
{
  vtkImageIterator<T> inIt(inData, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  float luminance;

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
    {
      // now process the components
      luminance = 0.30 * *inSI++;
      luminance += 0.59 * *inSI++;
      luminance += 0.11 * *inSI++;
      *outSI = static_cast<T>(luminance);
      ++outSI;
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}

//------------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The output data
// must match input type.  This method does handle boundary conditions.
void vtkImageLuminance::ThreadedExecute(
  vtkImageData* inData, vtkImageData* outData, int outExt[6], int id)
{
  vtkDebugMacro(<< "Execute: inData = " << inData << ", outData = " << outData);

  // this filter expects that input is the same type as output.
  if (inData->GetNumberOfScalarComponents() != 3)
  {
    vtkErrorMacro(<< "Execute: input must have 3 components, but has "
                  << inData->GetNumberOfScalarComponents());
    return;
  }

  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
  {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
                  << ", must match out ScalarType " << outData->GetScalarType());
    return;
  }

  switch (inData->GetScalarType())
  {
    vtkTemplateMacro(
      vtkImageLuminanceExecute(this, inData, outData, outExt, id, static_cast<VTK_TT*>(nullptr)));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}
VTK_ABI_NAMESPACE_END
