// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageEuclideanToPolar.h"

#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageEuclideanToPolar);

//------------------------------------------------------------------------------
vtkImageEuclideanToPolar::vtkImageEuclideanToPolar()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->ThetaMaximum = 255.0;
}

//------------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageEuclideanToPolarExecute(vtkImageEuclideanToPolar* self, vtkImageData* inData,
  vtkImageData* outData, int outExt[6], int id, T*)
{
  vtkImageIterator<T> inIt(inData, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  double X, Y, Theta, R;
  double thetaMax = self->GetThetaMaximum();

  // find the region to loop over
  int maxC = inData->GetNumberOfScalarComponents();

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
    {
      // Pixel operation
      X = static_cast<double>(*inSI);
      Y = static_cast<double>(inSI[1]);

      if ((X == 0.0) && (Y == 0.0))
      {
        Theta = 0.0;
        R = 0.0;
      }
      else
      {
        Theta = atan2(Y, X) * thetaMax / (2.0 * vtkMath::Pi());
        if (Theta < 0.0)
        {
          Theta += thetaMax;
        }
        R = sqrt(X * X + Y * Y);
      }

      *outSI = static_cast<T>(Theta);
      outSI[1] = static_cast<T>(R);
      inSI += maxC;
      outSI += maxC;
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}

//------------------------------------------------------------------------------
void vtkImageEuclideanToPolar::ThreadedExecute(
  vtkImageData* inData, vtkImageData* outData, int outExt[6], int id)
{
  vtkDebugMacro(<< "Execute: inData = " << inData << ", outData = " << outData);

  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
  {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
                  << ", must match out ScalarType " << outData->GetScalarType());
    return;
  }

  // input must have at least two components
  if (inData->GetNumberOfScalarComponents() < 2)
  {
    vtkErrorMacro(<< "Execute: input does not have at least two components");
    return;
  }

  switch (inData->GetScalarType())
  {
    vtkTemplateMacro(vtkImageEuclideanToPolarExecute(
      this, inData, outData, outExt, id, static_cast<VTK_TT*>(nullptr)));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}

void vtkImageEuclideanToPolar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Maximum Angle: " << this->ThetaMaximum << "\n";
}
VTK_ABI_NAMESPACE_END
