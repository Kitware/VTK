/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDuotone.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkImageDuotone.h"



//----------------------------------------------------------------------------
// Description:
// Constructor sets default values
vtkImageDuotone::vtkImageDuotone()
{
  this->InputMaximum = 255.0;
  this->OutputMaximum = 255.0;
  this->Ink0[0] = 255;
  this->Ink0[1] = 0;
  this->Ink0[2] = 0;  
  this->Ink1[0] = 0;
  this->Ink1[1] = 255;
  this->Ink1[2] = 255;  
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_COMPONENT_AXIS);

  // 2 dimensions + Components
  this->ExecuteDimensionality = 3;
  // Vector operation.
  this->Dimensionality = 1;
}

void vtkImageDuotone::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageTwoOutputFilter::PrintSelf(os, indent);
  os << "Ink0: (" << this->Ink0[0] << ", " << this->Ink0[1] << ", " 
     << this->Ink0[2] << ")\n";
  os << "Ink1: (" << this->Ink1[0] << ", " << this->Ink1[1] << ", " 
     << this->Ink1[2] << ")\n";  
  os << "InputMaximum:" << this->InputMaximum << "\n";
  os << "OutputMaximum:" << this->OutputMaximum << "\n";
}




//----------------------------------------------------------------------------
// Description:
// The output image extent has no components.
void vtkImageDuotone::ComputeOutputImageInformation(vtkImageRegion *inRegion,
						    vtkImageRegion *outRegion)
{
  outRegion->SetImageExtent(inRegion->GetImageExtent());
  outRegion->SetAxisImageExtent(VTK_IMAGE_COMPONENT_AXIS, 0, 0);
}


//----------------------------------------------------------------------------
// Description:
// We need RGB values (components) for each pixel of the output.
void 
vtkImageDuotone::ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
						  vtkImageRegion *inRegion)
{
  inRegion->SetExtent(outRegion->GetImageExtent());
  inRegion->SetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, 0, 2);
}




//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageDuotoneExecute(vtkImageDuotone *self,
			    vtkImageRegion *inRegion, T *inPtr,
			    vtkImageRegion *out0Region, T *out0Ptr,
			    vtkImageRegion *out1Region, T *out1Ptr,
			    float v00, float v01, float v11)
{
  int min0, max0, min1, max1;
  int idx0, idx1, idxV;
  int inInc0, inInc1, inIncV;
  int out0Inc0, out0Inc1;
  int out1Inc0, out1Inc1;
  T *inPtr0, *inPtr1, *inPtrV;
  T *out0Ptr0, *out0Ptr1;
  T *out1Ptr0, *out1Ptr1;
  float factor, d0, d1, temp, outMax, inMax;
  float ink0[3], ink1[3];

  // Have to take dot product with inks and input pixels.
  self->GetInk0(ink0);
  self->GetInk1(ink1);
  // Take inverse of colors to make problem additive.
  inMax = self->GetInputMaximum();
  for (idxV = 0; idxV < 3; ++idxV)
    {
    ink0[idxV] = 1.0 - (ink0[idxV] / inMax);
    ink1[idxV] = 1.0 - (ink1[idxV] / inMax);
    }
  
  // accounts for divisor in equations ...
  // Also accounts for desired output scale, and dot product with input.
  outMax = self->GetOutputMaximum();
  factor = outMax / (inMax * (v01 * v01 - v00 * v11));
  
  // Get information to march through data 
  inRegion->GetIncrements(inInc0, inInc1);
  inRegion->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, inIncV);
  out0Region->GetIncrements(out0Inc0, out0Inc1);
  out1Region->GetIncrements(out1Inc0, out1Inc1);
  // Extents are the same for the two outputs
  out0Region->GetExtent(min0, max0, min1, max1);
  
  // Loop through ouput pixels
  inPtr1 = inPtr;
  out0Ptr1 = out0Ptr;
  out1Ptr1 = out1Ptr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    inPtr0 = inPtr1;
    out0Ptr0 = out0Ptr1;
    out1Ptr0 = out1Ptr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      // Compute Dot product of vector with inks.
      d0 = d1 = 0.0;
      inPtrV = inPtr0;
      for (idxV = 0; idxV <= 2; ++idxV)
	{
	// Subtractive is a pain.
	d0 += (inMax - (float)(*inPtrV)) * ink0[idxV];
	d1 += (inMax - (float)(*inPtrV)) * ink1[idxV];
	inPtrV += inIncV;
	}
      
      // Compute how much of ink0 is needed. (set two outputs)
      temp = (T)((d1*v01 - d0*v11) * factor);
      // Clamp from 0 to outMax 
      temp = temp < 0.0 ? 0.0 : temp;
      temp = temp > outMax ? outMax : temp;
      // Ken said 0 is alot of ink, so switch this around
      *out0Ptr0 = (T)(outMax - temp);
      
      // Compute how much of ink1 is needed. (set two outputs)
      temp = (T)((d0*v01 - d1*v00) * factor);
      // Clamp from 0 to outMax
      temp = temp < 0.0 ? 0.0 : temp;
      temp = temp > outMax ? outMax : temp;
      // Ken said 0 is alot of ink, so switch this around
      *out1Ptr0 = (T)(outMax - temp);
      
      out0Ptr0 += out0Inc0;
      out1Ptr0 += out1Inc0;
      inPtr0 += inInc0;
      }
    out0Ptr1 += out0Inc1;
    out1Ptr1 += out1Inc1;
    inPtr1 += inInc1;
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageDuotone::Execute(vtkImageRegion *inRegion, 
			      vtkImageRegion *outRegion0,
			      vtkImageRegion *outRegion1)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr0 = outRegion0->GetScalarPointer();
  void *outPtr1 = outRegion1->GetScalarPointer();
  float v00, v01, v11;
  int idx;
  
  // this filter expects that input is the same type as outputs.
  if (inRegion->GetScalarType() != outRegion0->GetScalarType() ||
      inRegion->GetScalarType() != outRegion1->GetScalarType())      
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inRegion->GetScalarType()
            << ", must match out ScalarTypes " << outRegion0->GetScalarType()
            << " and " << outRegion1->GetScalarType());
    return;
    }
  
  // Compute factors for minimizing errors
  v00 = v01 = v11 = 0.0;
  for (idx = 0; idx < 3; ++idx)
    { // take opposite color to make problem additive.
    v00 += (this->InputMaximum-this->Ink0[idx]) 
      * (this->InputMaximum-this->Ink0[idx]);
    v01 += (this->InputMaximum-this->Ink0[idx]) 
      * (this->InputMaximum-this->Ink1[idx]);
    v11 += (this->InputMaximum-this->Ink1[idx]) 
      * (this->InputMaximum-this->Ink1[idx]);
    }
  // consider input maximum.
  v00 = v00 / (this->InputMaximum * this->InputMaximum);
  v01 = v01 / (this->InputMaximum * this->InputMaximum);
  v11 = v11 / (this->InputMaximum * this->InputMaximum);

  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageDuotoneExecute(this, inRegion, (float *)(inPtr), 
			     outRegion0, (float *)(outPtr0),
			     outRegion1, (float *)(outPtr1),
			     v00, v01, v11);
      break;
    case VTK_INT:
      vtkImageDuotoneExecute(this, inRegion, (int *)(inPtr), 
			     outRegion0, (int *)(outPtr0),
			     outRegion1, (int *)(outPtr1),
			     v00, v01, v11);
      break;
    case VTK_SHORT:
      vtkImageDuotoneExecute(this, inRegion, (short *)(inPtr), 
			     outRegion0, (short *)(outPtr0),
			     outRegion1, (short *)(outPtr1),
			     v00, v01, v11);			     
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageDuotoneExecute(this, inRegion, (unsigned short *)(inPtr), 
			     outRegion0, (unsigned short *)(outPtr0),
			     outRegion1, (unsigned short *)(outPtr1),
			     v00, v01, v11);			     
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageDuotoneExecute(this, inRegion, (unsigned char *)(inPtr), 
			     outRegion0, (unsigned char *)(outPtr0),
			     outRegion1, (unsigned char *)(outPtr1),
			     v00, v01, v11);			     
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
















