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

#include <math.h>

vtkCxxRevisionMacro(vtkImageEuclideanToPolar, "1.21");
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
                                    vtkImageData *inData, T *inPtr,
                                    vtkImageData *outData, T *outPtr,
                                    int outExt[6], int id)
{
  int idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  float X, Y, Theta, R;
  float thetaMax = self->GetThetaMaximum();
  
  // find the region to loop over
  maxC = inData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0]; 
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!id) 
        {
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      for (idxX = 0; idxX <= maxX; idxX++)
        {
        // Pixel operation
        X = (float)(*inPtr);
        Y = (float)(inPtr[1]);

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
        
        *outPtr = (T)(Theta);
        outPtr[1] = (T)(R);
        inPtr += maxC;
        outPtr += maxC;
        }
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}

//----------------------------------------------------------------------------
void vtkImageEuclideanToPolar::ThreadedExecute(vtkImageData *inData, 
                                       vtkImageData *outData,
                                       int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
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
    vtkTemplateMacro7(vtkImageEuclideanToPolarExecute, this, 
                      inData, (VTK_TT *)(inPtr), 
                      outData, (VTK_TT *)(outPtr), outExt, id);
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
