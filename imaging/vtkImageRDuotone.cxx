/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRDuotone.cxx
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
#include "vtkImageRDuotone.h"



//----------------------------------------------------------------------------
vtkImageRDuotone::vtkImageRDuotone()
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

void vtkImageRDuotone::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageTwoInputFilter::PrintSelf(os, indent);
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
void vtkImageRDuotone::ComputeOutputImageInformation(vtkImageRegion *inRegion0,
						     vtkImageRegion *inRegion1,
						     vtkImageRegion *outRegion)
{
  inRegion0 = inRegion0;
  outRegion->SetImageExtent(inRegion1->GetImageExtent());
  outRegion->SetAxisImageExtent(VTK_IMAGE_COMPONENT_AXIS, 0, 2);
}


//----------------------------------------------------------------------------
// Description:
// We need RGB values (components) for each pixel of the output.
void 
vtkImageRDuotone::ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
						   vtkImageRegion *inRegion0,
						   vtkImageRegion *inRegion1)
{
  inRegion0->SetExtent(outRegion->GetImageExtent());
  inRegion0->SetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, 0, 0);
  inRegion1->SetExtent(outRegion->GetImageExtent());
  inRegion1->SetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, 0, 0);
}




//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageRDuotoneExecute(vtkImageRDuotone *self,
			     vtkImageRegion *in0Region, T *in0Ptr,
			     vtkImageRegion *in1Region, T *in1Ptr,
			     vtkImageRegion *outRegion, T *outPtr)
{
  float ink0[3], ink1[3];
  int min0, max0, min1, max1;
  int idx0, idx1, idxV;
  int in0Inc0, in0Inc1;
  int in1Inc0, in1Inc1;
  int outInc0, outInc1, outIncV;
  T *in0Ptr0, *in0Ptr1;
  T *in1Ptr0, *in1Ptr1;
  T *outPtr0, *outPtr1, *outPtrV;
  float inMax, outMax, temp;
  
  inMax = self->GetInputMaximum();
  outMax = self->GetOutputMaximum();
  // Invert color to gvet into additive mode.
  self->GetInk0(ink0);
  self->GetInk1(ink1);
  for (idxV = 0; idxV < 3; ++idxV)
    {
    // input to output coversion built in.
    ink0[idxV] = (outMax - ink0[idxV]) / inMax;
    ink1[idxV] = (outMax - ink1[idxV]) / inMax;
    }
  
  // Get information to march through data 
  in0Region->GetIncrements(in0Inc0, in0Inc1);
  in1Region->GetIncrements(in1Inc0, in1Inc1);
  outRegion->GetIncrements(outInc0, outInc1);
  outRegion->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, outIncV);
  outRegion->GetExtent(min0, max0, min1, max1);

  // Loop through ouput pixels
  in0Ptr1 = in0Ptr;
  in1Ptr1 = in1Ptr;
  outPtr1 = outPtr;
  for (idx1 = min0; idx1 <= max1; ++idx1)
    {
    outPtr0 = outPtr1;
    in0Ptr0 = in0Ptr1;
    in1Ptr0 = in1Ptr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      
      outPtrV = outPtr0;
      for (idxV = 0; idxV < 3; ++idxV)
	{
	// Don't even try to figure it out ...
	temp = outMax - (inMax * (ink0[idxV] + ink1[idxV]));
	temp += ((*in0Ptr0) * ink0[idxV] + (*in1Ptr0) * ink1[idxV]);
	temp = temp < 0.0 ? 0.0 : temp;
	*outPtrV =  (T)(temp > outMax ? outMax : temp);
	
	outPtrV += outIncV;
	}
      
      outPtr0 += outInc0;
      in0Ptr0 += in0Inc0;
      in1Ptr0 += in1Inc0;
      }
    outPtr1 += outInc1;
    in0Ptr1 += in0Inc1;
    in1Ptr1 += in1Inc1;
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageRDuotone::Execute(vtkImageRegion *inRegion0, 
			       vtkImageRegion *inRegion1, 
			       vtkImageRegion *outRegion)
{
  void *in0Ptr = inRegion0->GetScalarPointer();
  void *in1Ptr = inRegion1->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  // this filter expects that inputs are the same type as output.
  if (inRegion0->GetScalarType() != outRegion->GetScalarType() ||
      inRegion1->GetScalarType() != outRegion->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarTypes, " 
       << inRegion0->GetScalarType() << " and " << inRegion1->GetScalarType()
       << ", must match out ScalarType " << outRegion->GetScalarType());
    return;
    }
  
  switch (inRegion0->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageRDuotoneExecute(this, 
			  inRegion0, (float *)(in0Ptr), 
			  inRegion1, (float *)(in1Ptr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageRDuotoneExecute(this, 
			  inRegion0, (int *)(in0Ptr), 
			  inRegion1, (int *)(in1Ptr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageRDuotoneExecute(this, 
			  inRegion0, (short *)(in0Ptr), 
			  inRegion1, (short *)(in1Ptr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageRDuotoneExecute(this, 
			  inRegion0, (unsigned short *)(in0Ptr), 
			  inRegion1, (unsigned short *)(in1Ptr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageRDuotoneExecute(this, 
			  inRegion0, (unsigned char *)(in0Ptr), 
			  inRegion1, (unsigned char *)(in1Ptr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
















