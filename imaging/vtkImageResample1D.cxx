/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResample1D.cxx
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
#include <math.h>
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageResample1D.h"



//----------------------------------------------------------------------------
// Description:
// Constructor: Sets default filter to be identity.
vtkImageResample1D::vtkImageResample1D()
{
  this->FilteredAxis = VTK_IMAGE_X_AXIS;
  this->SetExecutionAxes(VTK_IMAGE_X_AXIS);
  // For better performance, the execute function was written as a 2d.
  this->NumberOfExecutionAxes = 2;
  
  this->MagnificationFactor = 1.0;
  this->OutputSpacing = 0.0; // not specified.
}


//----------------------------------------------------------------------------
void vtkImageResample1D::SetFilteredAxis(int axis)
{
  if (axis < 0 || axis > 3)
    {
    vtkErrorMacro("SetFilteredAxis: Bad axis " << axis);
    return;
    }
  if (axis != this->FilteredAxis)
    {
    this->FilteredAxis = axis;
    this->Modified();
    this->SetExecutionAxes(axis);
    this->NumberOfExecutionAxes = 2;
    }
}

  
//----------------------------------------------------------------------------
void vtkImageResample1D::SetOutputSpacing(float spacing)
{
  if (this->OutputSpacing != spacing)
    {
    this->OutputSpacing = spacing;
    this->Modified();
    if (spacing != 0.0)
      {
      // Delay computing the magnification factor.
      // Input might not be set yet.
      this->MagnificationFactor = 0.0; // Not computed yet.
      }
    }
}


//----------------------------------------------------------------------------
void vtkImageResample1D::SetMagnificationFactor(float factor)
{
  this->MagnificationFactor = factor;
  // Spacing is no longer valid.
  this->OutputSpacing = 0.0; // Not computed yet.
}

//----------------------------------------------------------------------------
float vtkImageResample1D::GetMagnificationFactor()
{
  if (this->MagnificationFactor == 0.0)
    {
    float inputSpacing;
    if ( ! this->Input)
      {
      vtkErrorMacro("GetMagnificationFactor: Input not set.");
      return 0.0;
      }
    this->Input->UpdateImageInformation();
    this->Input->GetAxisSpacing(this->FilteredAxis, inputSpacing);
    this->MagnificationFactor = this->OutputSpacing / inputSpacing;
    }
  
  return this->MagnificationFactor;
}




//----------------------------------------------------------------------------
// Description:
// This method computes the Region of input necessary to generate outRegion.
// It assumes offset and size are multiples of Magnify Factors.
void vtkImageResample1D::ComputeRequiredInputUpdateExtent(vtkImageCache *out,
							  vtkImageCache *in)
{
  int min, max;
 
  out->GetAxisUpdateExtent(this->FilteredAxis, min, max);
  
  min = (int)(floor((float)(min) / this->GetMagnificationFactor()));
  max = (int)(ceil((float)(max) / this->GetMagnificationFactor()));

  in->SetAxisUpdateExtent(this->FilteredAxis, min, max);
}


//----------------------------------------------------------------------------
// Description:
// Computes any global image information associated with regions.
void vtkImageResample1D::ExecuteImageInformation(vtkImageCache *in, 
						 vtkImageCache *out)
{
  int wholeMin, wholeMax;
  float spacing;

  in->GetAxisWholeExtent(this->FilteredAxis, wholeMin, wholeMax);
  in->GetAxisSpacing(this->FilteredAxis,spacing);

  // Scale the output extent
  wholeMin = (int)(ceil((float)(wholeMin) * this->GetMagnificationFactor()));
  wholeMax = (int)(floor((float)(wholeMax) * this->GetMagnificationFactor()));

  // Change the data spacing
  spacing /= this->GetMagnificationFactor();

  out->SetAxisWholeExtent(this->FilteredAxis, wholeMin, wholeMax);
  out->SetAxisSpacing(this->FilteredAxis, spacing);
  
  // just in case  the input spacing has changed.
  if (this->OutputSpacing != 0.0)
    {
    // Cause MagnificationFactor to recompute.
    this->MagnificationFactor = 0.0;
    }
}



//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
// 2d even though operation is 1d.
// Note: Slight misalignment (pixel replication is not nearest neighbor).
template <class T>
static void vtkImageResample1DExecute(vtkImageResample1D *self,
			      vtkImageRegion *inRegion, T *inPtr,
			      vtkImageRegion *outRegion, T *outPtr)
{
  int outMin0, outMax0, outMin1, outMax1, inMin0, inMax0;
  int outIdx0, outIdx1; 
  int inInc0, inInc1, outInc0, outInc1;
  T *inPtr0, *inPtr1, *outPtr0, *outPtr1;
  float magFactor;
  float val, valStep, f, fStep;

  // Get information to march through data 
  inRegion->GetIncrements(inInc0, inInc1);
  outRegion->GetIncrements(outInc0, outInc1);
  inRegion->GetExtent(inMin0, inMax0);
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1);

  // interpolation stuff
  magFactor = self->GetMagnificationFactor();
  
  // Loop through output pixels
  inPtr1 = inPtr;
  outPtr1 = outPtr;
  for (outIdx1 = outMin1; outIdx1 <= outMax1; ++outIdx1)
    {
    inPtr0 = inPtr1;
    outPtr0 = outPtr1;
    if (inMin0 == inMax0)
      { // the input only has one slice
      val = *inPtr0;
      }
    else
      { // Setup interpolation loop parameters
      // Get just the decimal of the in index
      f = (float)(outMin0) / magFactor;
      f -= floor(f);
      // interpolate start value
      valStep = *(inPtr0+inInc0) - *inPtr0;
      val = *inPtr0 + valStep * f;
      // Compute how f changes and val changes for each iteration.
      fStep = 1.0 / magFactor;
      valStep = fStep * valStep;
      }
    for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
      {
      *outPtr0 = (T)(val);

      outPtr0 += outInc0;
      
      // Update interpolation loop parameters.
      f += fStep;
      if (f <= 1.0)
	{
	val += valStep;
	}
      else
	{ // crossed border, need to compute new start and steps.
	f -= 1.0;
	// interpolate start value
	inPtr0 += inInc0;
	valStep = *(inPtr0+inInc0) - *inPtr0;
	val = *inPtr0 + valStep * f;
	// Compute how f changes and val changes for each iteration.
	fStep = 1.0 / magFactor;
	valStep = fStep * valStep;
	}
      }
    inPtr1 += inInc1;
    outPtr1 += outInc1;
    }
}

    
//----------------------------------------------------------------------------
// Description:
// This method uses the input region to fill the output region.
// It can handle any type data, but the two regions must have the same 
// data type.
void vtkImageResample1D::Execute(vtkImageRegion *inRegion, 
				 vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects that input is the same type as output.
  if (inRegion->GetScalarType() != outRegion->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inRegion->GetScalarType()
         << ", must match out ScalarType " << outRegion->GetScalarType());
    return;
    }
  
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageResample1DExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageResample1DExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageResample1DExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageResample1DExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageResample1DExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
















