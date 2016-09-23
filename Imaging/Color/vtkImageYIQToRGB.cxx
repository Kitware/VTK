/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageYIQToRGB.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageYIQToRGB.h"

#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkImageYIQToRGB);

//----------------------------------------------------------------------------
vtkImageYIQToRGB::vtkImageYIQToRGB()
{
  this->Maximum = 255.0;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageYIQToRGBExecute(vtkImageYIQToRGB *self,
                             vtkImageData *inData,
                             vtkImageData *outData,
                             int outExt[6], int id, T *)
{
  vtkImageIterator<T> inIt(inData, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  int maxC;
  double R, G, B, Y, I, Q;
  double max = self->GetMaximum();

  // find the region to loop over
  maxC = inData->GetNumberOfScalarComponents()-1;

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
    {
      // Pixel operation
      Y = static_cast<double>(*inSI) / max; inSI++;
      I = static_cast<double>(*inSI) / max; inSI++;
      Q = static_cast<double>(*inSI) / max; inSI++;

      //vtkMath::RGBToHSV(R, G, B, &H, &S, &V);
      // Port this snippet below into vtkMath as YIQToRGB(similar to RGBToHSV)
      // The numbers used below are standard numbers used from here
      // http://www.cs.rit.edu/~ncs/color/t_convert.html
      // Please do not change these numbers
      R = 1*Y + 0.956*I + 0.621*Q;
      G = 1*Y - 0.272*I - 0.647*Q;
      B = 1*Y - 1.105*I + 1.702*Q;
      //----------------------------------------------------------------

      R *= max;
      G *= max;
      B *= max;

      if (R > max)
      {
        R = max;
      }
      if (G > max)
      {
        G = max;
      }
      if (B > max)
      {
        B = max;
      }

      // assign output.
      *outSI = static_cast<T>(R); outSI++;
      *outSI = static_cast<T>(G); outSI++;
      *outSI = static_cast<T>(B); outSI++;

      for (int idxC = 3; idxC <= maxC; idxC++)
      {
        *outSI++ = *inSI++;
      }
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}

//----------------------------------------------------------------------------
void vtkImageYIQToRGB::ThreadedExecute (vtkImageData *inData,
                                         vtkImageData *outData,
                                         int outExt[6], int id)
{
  vtkDebugMacro(<< "Execute: inData = " << inData
  << ", outData = " << outData);

  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
  {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
    << ", must match out ScalarType " << outData->GetScalarType());
    return;
  }

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
      vtkImageYIQToRGBExecute( this, inData,
                               outData, outExt, id,
                               static_cast<VTK_TT *>(0)));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}

void vtkImageYIQToRGB::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Maximum: " << this->Maximum << "\n";
}
