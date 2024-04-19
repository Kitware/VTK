// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageRGBToXYZ.h"

#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageRGBToXYZ);

namespace
{
//------------------------------------------------------------------------------
template <class T>
void vtkImageRGBToXYZExecute(
  vtkImageRGBToXYZ* self, vtkImageData* inData, vtkImageData* outData, int outExt[6], int id, T*)
{
  vtkImageIterator<T> inIt(inData, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  double r, g, b, x, y, z;

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
    {
      r = *inSI;
      ++inSI;
      g = *inSI;
      ++inSI;
      b = *inSI;
      ++inSI;

      vtkMath::RGBToXYZ(r, g, b, &x, &y, &z);

      // assign output.
      *outSI = x;
      outSI++;
      *outSI = y;
      outSI++;
      *outSI = z;
      outSI++;
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}
} // anonymoun namespace

//------------------------------------------------------------------------------
vtkImageRGBToXYZ::vtkImageRGBToXYZ()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
void vtkImageRGBToXYZ::ThreadedExecute(
  vtkImageData* inData, vtkImageData* outData, int outExt[6], int id)
{
  vtkDebugMacro(<< "Execute: inData = " << inData << ", outData = " << outData);

  // need three components for input and output
  if (inData->GetNumberOfScalarComponents() < 3)
  {
    vtkErrorMacro("Input has too few components");
    return;
  }
  if (outData->GetNumberOfScalarComponents() < 3)
  {
    vtkErrorMacro("Output has too few components");
    return;
  }

  switch (inData->GetScalarType())
  {
    vtkTemplateMacro(
      vtkImageRGBToXYZExecute(this, inData, outData, outExt, id, static_cast<VTK_TT*>(nullptr)));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}

void vtkImageRGBToXYZ::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
