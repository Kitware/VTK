/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRGBToHSV.cxx
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
#include "vtkImageRGBToHSV.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageRGBToHSV, "1.21");
vtkStandardNewMacro(vtkImageRGBToHSV);

//----------------------------------------------------------------------------
vtkImageRGBToHSV::vtkImageRGBToHSV()
{
  this->Maximum = 255.0;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageRGBToHSVExecute(vtkImageRGBToHSV *self,
                                    vtkImageData *inData, T *inPtr,
                                    vtkImageData *outData, T *outPtr,
                                    int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  float R, G, B, H, S, V;
  float max = self->GetMaximum();
  float temp;
  
  // find the region to loop over
  maxC = inData->GetNumberOfScalarComponents()-1;
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
        R = (float)(*inPtr); inPtr++;
        G = (float)(*inPtr); inPtr++;
        B = (float)(*inPtr); inPtr++;
        temp = (float)(R + G + B);
        // Value is easy
        V = temp / 3.0;
        
        // Hue
        temp = sqrt((R-G)*(R-G) + (R-B)*(G-B));
        if(temp != 0.0)
          {
          temp = acos((0.5 * ((R-G) + (R-B))) / temp);
          }
        if (G >= B)
          {
          H = max * (temp / 6.2831853);
          }
        else
          {
          H = max * (1.0 - (temp / 6.2831853));
          }
        
        // Saturation
        temp = R;
        if (G < temp)
          {
          temp = G;
          }
        if (B < temp)
          {
          temp = B;
          }
        float sumRGB = R+G+B;
        if(sumRGB == 0.0)
          {
          S = 0.0;
          }
        else
          {
          S = max * (1.0 - (3.0 * temp / sumRGB));
          }
        
        // assign output.
        *outPtr = (T)(H); outPtr++;
        *outPtr = (T)(S); outPtr++;
        *outPtr = (T)(V); outPtr++;

        for (idxC = 3; idxC <= maxC; idxC++)
          {
          *outPtr++ = *inPtr++;
          }
        }
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}

//----------------------------------------------------------------------------
void vtkImageRGBToHSV::ThreadedExecute(vtkImageData *inData, 
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
    vtkTemplateMacro7(vtkImageRGBToHSVExecute, this, inData,(VTK_TT *)(inPtr), 
                      outData, (VTK_TT *)(outPtr), outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

void vtkImageRGBToHSV::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Maximum: " << this->Maximum << "\n";
}

