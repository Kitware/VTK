/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFourierCenter1D.cxx
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
#include "vtkImageFourierCenter1D.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageFourierCenter1D fitler.
vtkImageFourierCenter1D::vtkImageFourierCenter1D()
{
  // mimic a call to SetFilteredAxis.
  this->FilteredAxis = VTK_IMAGE_X_AXIS;
  this->SetExecutionAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_COMPONENT_AXIS);
  
  // Output is always floats.
  this->SetOutputScalarType(VTK_FLOAT);
}


//----------------------------------------------------------------------------
void vtkImageFourierCenter1D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFourierFilter::PrintSelf(os,indent);

  os << indent << "FilteredAxis: " << vtkImageAxisNameMacro(this->FilteredAxis)
     << "\n";
}

//----------------------------------------------------------------------------
// Description:
// Which axis will be operated on.
void vtkImageFourierCenter1D::SetFilteredAxis(int axis)
{  
  if (this->FilteredAxis == axis)
    {
    return;
    }
  
  if (axis < 0 || axis > 3)
    {
    vtkErrorMacro("SetFilteredAxis: Bad axis: " << axis);
    return;
    }
  
  // Tell the supper class which axes to loop over
  this->SetExecutionAxes(axis, VTK_IMAGE_COMPONENT_AXIS);
  
  this->FilteredAxis = axis;
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
// This method tells the superclass which input extent is needed.
// This gets the whole input (even though it may not be needed).
void 
vtkImageFourierCenter1D::ComputeRequiredInputUpdateExtent(vtkImageCache *out, 
							  vtkImageCache *in)
{
  int minWhole, maxWhole;

  out = out;
  in->GetAxisWholeExtent(this->FilteredAxis, minWhole, maxWhole);
  in->SetAxisUpdateExtent(this->FilteredAxis, minWhole, maxWhole);
}

//----------------------------------------------------------------------------
// Description:
// This method is passed input and output regions, and executes the fft
// algorithm to fill the output from the input.
void vtkImageFourierCenter1D::Execute(vtkImageRegion *inRegion, 
				      vtkImageRegion *outRegion)
{
  float *inPtr, *outPtr, *outPtrV;
  int wholeMin, wholeMax, mid;
  int outIdx, inIdx, idxV;
  int min, max, minV, maxV, inInc, inIncV, outInc, outIncV;
  
  // this filter expects that the output be floats.
  if (outRegion->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: Output must be be type float.");
    return;
    }
  // this filter expects that the input be floats.
  if (inRegion->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: Input must be be type float.");
    return;
    }

  // Determine the mid
  outRegion->GetWholeExtent(wholeMin, wholeMax);
  mid = (wholeMin + wholeMax) / 2;
  outRegion->GetExtent(min, max, minV, maxV);
  outRegion->GetIncrements(outInc, outIncV);
  inRegion->GetIncrements(inInc, inIncV);
  
  outPtr = (float *)(outRegion->GetScalarPointer());
  for (outIdx = min; outIdx <= max; ++outIdx)
    {
    // get the correct input (slow but easy)
    inIdx = outIdx + mid;
    if (inIdx > wholeMax)
      {
      inIdx -= (wholeMax - wholeMin + 1);
      }
    inPtr = (float *)(inRegion->GetScalarPointer(inIdx));

    // Copy all components
    outPtrV = outPtr;
    for (idxV = minV; idxV <= maxV; ++idxV)
      {
      *outPtrV = *inPtr;
      inPtr += inIncV;
      outPtrV += outIncV;
      }
    
    outPtr += outInc;
    }
}



















