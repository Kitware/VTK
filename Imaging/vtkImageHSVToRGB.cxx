/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHSVToRGB.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>
#include "vtkImageHSVToRGB.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageHSVToRGB* vtkImageHSVToRGB::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageHSVToRGB");
  if(ret)
    {
    return (vtkImageHSVToRGB*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageHSVToRGB;
}






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
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "Maximum: " << this->Maximum << "\n";
}

