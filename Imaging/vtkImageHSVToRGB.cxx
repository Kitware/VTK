/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHSVToRGB.cxx
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
#include "vtkImageHSVToRGB.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageHSVToRGB, "1.23");
vtkStandardNewMacro(vtkImageHSVToRGB);

//----------------------------------------------------------------------------
vtkImageHSVToRGB::vtkImageHSVToRGB()
{
  this->Maximum = 255.0;
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageHSVToRGBExecute(vtkImageHSVToRGB *self,
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
  float third = max / 3.0;
  
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
        H = (float)(*inPtr); inPtr++;
        S = (float)(*inPtr); inPtr++;
        V = (float)(*inPtr); inPtr++;

        // compute rgb assuming S = 1.0;
        if (H >= 0.0 && H <= third) // red -> green
          {
          G = H/third;
          R = 1.0 - G;
          B = 0.0;
          }
        else if (H >= third && H <= 2.0*third) // green -> blue
          {
          B = (H - third)/third;
          G = 1.0 - B;
          R = 0.0;
          }
        else // blue -> red
          {
          R = (H - 2.0 * third)/third;
          B = 1.0 - R;
          G = 0.0;
          }
        
        // add Saturation to the equation.
        S = S / max;
        //R = S + (1.0 - S)*R;
        //G = S + (1.0 - S)*G;
        //B = S + (1.0 - S)*B;
        // what happend to this?
        R = S*R + (1.0 - S);
        G = S*G + (1.0 - S);
        B = S*B + (1.0 - S);
        
        // Use value to get actual RGB 
        // normalize RGB first then apply value
        temp = R + G + B; 
        //V = 3 * V / (temp * max);
        // and what happend to this?
        V = 3 * V / (temp);
        R = R * V;
        G = G * V;
        B = B * V;
        
        // clip below 255
        //if (R > 255.0) R = max;
        //if (G > 255.0) G = max;
        //if (B > 255.0) B = max;
        // mixed constant 255 and max ?????
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
        *outPtr = (T)(R); outPtr++;
        *outPtr = (T)(G); outPtr++;
        *outPtr = (T)(B); outPtr++;

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
void vtkImageHSVToRGB::ThreadedExecute(vtkImageData *inData, 
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
    vtkTemplateMacro7(vtkImageHSVToRGBExecute,this, inData, (VTK_TT *)(inPtr), 
                      outData, (VTK_TT *)(outPtr), outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

void vtkImageHSVToRGB::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Maximum: " << this->Maximum << "\n";
}

