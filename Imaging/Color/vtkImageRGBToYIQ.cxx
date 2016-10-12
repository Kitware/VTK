/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRGBToYIQ.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageRGBToYIQ.h"

#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkImageRGBToYIQ);

//----------------------------------------------------------------------------
vtkImageRGBToYIQ::vtkImageRGBToYIQ()
{
  this->Maximum = 255.0;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
namespace{
template <class T>
void vtkImageRGBToYIQExecute(vtkImageRGBToYIQ *self,
                             vtkImageData *inData,
                             vtkImageData *outData,
                             int outExt[6], int id, T *)
{
  vtkImageIterator<T> inIt(inData, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  int idxC, maxC;
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
      R = static_cast<double>(*inSI) / max; inSI++;
      G = static_cast<double>(*inSI) / max; inSI++;
      B = static_cast<double>(*inSI) / max; inSI++;

      //vtkMath::RGBToHSV(R, G, B, &H, &S, &V);
      // Port this snippet below into vtkMath as RGBtoYIQ(similar to RGBToHSV above)
      // The numbers used below are standard numbers used from here
      // https://www.eembc.org/techlit/datasheets/yiq_consumer.pdf
      // Please do not change these numbers
      Y = 0.299*R + 0.587*G + 0.114*B;
      I = 0.596*R - 0.275*G - 0.321*B;
      Q = 0.212*R - 0.523*G + 0.311*B;
      //----------------------------------------------------------------

      Y *= max;
      I *= max;
      Q *= max;

      if (Y > max)
      {
        Y = max;
      }
      if (I > max)
      {
        I = max;
      }
      if (Q > max)
      {
        Q = max;
      }

      // assign output.
      *outSI = static_cast<T>(Y); outSI++;
      *outSI = static_cast<T>(I); outSI++;
      *outSI = static_cast<T>(Q); outSI++;

      for (idxC = 3; idxC <= maxC; idxC++)
      {
        *outSI++ = *inSI++;
      }
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}
}


//----------------------------------------------------------------------------
void vtkImageRGBToYIQ::ThreadedExecute (vtkImageData *inData,
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
      vtkImageRGBToYIQExecute( this, inData,
                               outData, outExt, id,
                               static_cast<VTK_TT *>(0)));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}

void vtkImageRGBToYIQ::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Maximum: " << this->Maximum << "\n";
}
