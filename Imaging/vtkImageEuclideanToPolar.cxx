/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageEuclideanToPolar.cxx
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
#include "vtkImageEuclideanToPolar.h"
#include "vtkObjectFactory.h"
#include "vtkImageProgressIterator.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageEuclideanToPolar, "1.22");
vtkStandardNewMacro(vtkImageEuclideanToPolar);

//----------------------------------------------------------------------------
vtkImageEuclideanToPolar::vtkImageEuclideanToPolar()
{
  this->ThetaMaximum = 255.0;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageEuclideanToPolarExecute(vtkImageEuclideanToPolar *self,
                                    vtkImageData *inData,
                                    vtkImageData *outData,
                                    int outExt[6], int id, T *)
{
  vtkImageIterator<T> inIt(inData, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  float X, Y, Theta, R;
  float thetaMax = self->GetThetaMaximum();
  
  // find the region to loop over
  int maxC = inData->GetNumberOfScalarComponents();
  
  // Loop through ouput pixels
  while (!outIt.IsAtEnd())
    {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
      {
      // Pixel operation
      X = (float)(*inSI);
      Y = (float)(inSI[1]);
      
      if ((X == 0.0) && (Y == 0.0))
        {
        Theta = 0.0;
        R = 0.0;
        }
      else
        {
        Theta = atan2(Y, X) * thetaMax / 6.2831853;
        if (Theta < 0.0)
          {
          Theta += thetaMax;
          }
        R = sqrt(X*X + Y*Y);
        }
      
      *outSI = (T)(Theta);
      outSI[1] = (T)(R);
      inSI += maxC;
      outSI += maxC;
      }
    inIt.NextSpan();
    outIt.NextSpan();
    }
}

//----------------------------------------------------------------------------
void vtkImageEuclideanToPolar::ThreadedExecute(vtkImageData *inData, 
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
  
  // input must have at least two components
  if (inData->GetNumberOfScalarComponents() < 2)
    {
    vtkErrorMacro(<< "Execute: input does not have at least two components");
    return;
    }

  switch (inData->GetScalarType())
    {
    vtkTemplateMacro6(vtkImageEuclideanToPolarExecute, this, 
                      inData, outData, outExt, id, static_cast<VTK_TT *>(0));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

void vtkImageEuclideanToPolar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Maximum Angle: " << this->ThetaMaximum << "\n";
}
