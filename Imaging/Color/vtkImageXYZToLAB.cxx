// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageXYZToLAB.h"

#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include "vtkDoubleArray.h"
#include "vtkPointData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageXYZToLAB);

namespace
{
//------------------------------------------------------------------------------
template <class T>
void vtkImageXYZToLABExecute(
  vtkImageXYZToLAB* self, vtkImageData* inData, vtkImageData* outData, int outExt[6], int id, T*)
{
  vtkImageIterator<T> inIt(inData, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  double x, y, z, l, a, b;

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
    {
      x = *(inSI++);
      y = *(inSI++);
      z = *(inSI++);

      vtkMath::XYZToLab(x, y, z, &l, &a, &b);

      // assign output.
      *outSI = static_cast<T>(l);
      outSI++;
      *outSI = static_cast<T>(a);
      outSI++;
      *outSI = static_cast<T>(b);
      outSI++;
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}
} // anonymous namespace

//------------------------------------------------------------------------------
vtkImageXYZToLAB::vtkImageXYZToLAB()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
void vtkImageXYZToLAB::ThreadedExecute(
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
      vtkImageXYZToLABExecute(this, inData, outData, outExt, id, static_cast<VTK_TT*>(nullptr)));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}

void vtkImageXYZToLAB::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
