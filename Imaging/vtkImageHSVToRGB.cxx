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
#include "vtkImageProgressIterator.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageHSVToRGB, "1.25");
vtkStandardNewMacro(vtkImageHSVToRGB);

//----------------------------------------------------------------------------
vtkImageHSVToRGB::vtkImageHSVToRGB()
{
  this->Maximum = 255.0;
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageHSVToRGBExecute(vtkImageHSVToRGB *self,
                             vtkImageData *inData,
                             vtkImageData *outData,
                             int outExt[6], int id, T *)
{
  vtkImageIterator<T> inIt(inData, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  float R, G, B, H, S, V;
  float max = self->GetMaximum();
  float temp;
  float third = max / 3.0;
  int idxC;
  
  // find the region to loop over
  int maxC = inData->GetNumberOfScalarComponents()-1;
  
  // Loop through ouput pixels
  while (!outIt.IsAtEnd())
    {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
      {
      // Pixel operation
      H = (float)(*inSI); ++inSI;
      S = (float)(*inSI); ++inSI;
      V = (float)(*inSI); ++inSI;
      
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
      *outSI = (T)(R); ++outSI;
      *outSI = (T)(G); ++outSI;
      *outSI = (T)(B); ++outSI;
      
      for (idxC = 3; idxC <= maxC; idxC++)
        {
        *outSI++ = *inSI++;
        }
      }
    inIt.NextSpan();
    outIt.NextSpan();
    }
}

//----------------------------------------------------------------------------
void vtkImageHSVToRGB::ThreadedExecute(vtkImageData *inData, 
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
    vtkTemplateMacro6(vtkImageHSVToRGBExecute,this, inData, 
                      outData, outExt, id, static_cast<VTK_TT *>(0));
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

