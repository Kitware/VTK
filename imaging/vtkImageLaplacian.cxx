/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLaplacian.cxx
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
#include "vtkImageLaplacian.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageLaplacian fitler.
vtkImageLaplacian::vtkImageLaplacian()
{
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS);
  this->SetOutputScalarType(VTK_FLOAT);
  
  this->ExecuteDimensionality = 3;
  this->Dimensionality = 3;
}


//----------------------------------------------------------------------------
void vtkImageLaplacian::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageFilter::PrintSelf(os, indent);
}


//----------------------------------------------------------------------------
// Description:
// This method computes the input extent necessary to generate the output.
void vtkImageLaplacian::ComputeRequiredInputRegionExtent(
			vtkImageRegion *outRegion, vtkImageRegion *inRegion)
{
  int extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int *imageExtent;
  int idx;

  imageExtent = inRegion->GetImageExtent();
  outRegion->GetExtent(VTK_IMAGE_DIMENSIONS, extent);
  // Component axis is number 4
  extent[8] = 0;
  extent[9] = 0;
  
  // grow input image extent.
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    extent[idx*2] -= 1;
    extent[idx*2+1] += 1;
    // we must clip extent with image extent is we hanlde boundaries.
    if (extent[idx*2] < imageExtent[idx*2])
      {
      extent[idx*2] = imageExtent[idx*2];
      }
    if (extent[idx*2 + 1] > imageExtent[idx*2 + 1])
      {
      extent[idx*2 + 1] = imageExtent[idx*2 + 1];
      }
    }
  
  inRegion->SetExtent(VTK_IMAGE_DIMENSIONS, extent);
}





//----------------------------------------------------------------------------
// Description:
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
void vtkImageLaplacianExecute(vtkImageLaplacian *self,
			      vtkImageRegion *inRegion, T *inPtr, 
			      vtkImageRegion *outRegion, float *outPtr)
{
  int axisIdx, axesNum;
  float d, sum;
  float r[3];
  // For looping though output (and input) pixels.
  int min0, max0, min1, max1, min2, max2;
  int outIdx0, outIdx1, outIdx2;
  int outInc0, outInc1, outInc2;
  float *outPtr0, *outPtr1, *outPtr2;
  int inInc0, inInc1, inInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  // For computation of Laplacian (everything has to be arrays for loop).
  int *incs, *imageExtent, *idxs, outIdxs[3];
  
  // Get the dimensionality of the Laplacian.
  axesNum = self->GetDimensionality();
  
  // Get information to march through data (skip component)
  inRegion->GetIncrements(inInc0, inInc1, inInc2); 
  outRegion->GetIncrements(outInc0, outInc1, outInc2); 
  outRegion->GetExtent(min0,max0, min1,max1, min2,max2);
    
  // We want the input pixel to correspond to output
  inPtr = (T *)(inRegion->GetScalarPointer(min0,min1,min2));

  // The aspect ratio is important for computing the Laplacian.
  // Divid by dx twice (second derivative).
  inRegion->GetAspectRatio(4, r);
  r[0] = 1.0 / r[0] * r[0];
  r[1] = 1.0 / r[1] * r[1];
  r[2] = 1.0 / r[2] * r[2];
  
  // loop through pixels of output
  outPtr2 = outPtr;
  inPtr2 = inPtr;
  for (outIdx2 = min2; outIdx2 <= max2; ++outIdx2)
    {
    outIdxs[2] = outIdx2;
    outPtr1 = outPtr2;
    inPtr1 = inPtr2;
    for (outIdx1 = min1; outIdx1 <= max1; ++outIdx1)
      {
      outIdxs[1] = outIdx1;
      outPtr0 = outPtr1;
      inPtr0 = inPtr1;
      for (outIdx0 = min0; outIdx0 <= max0; ++outIdx0)
	{
	*outIdxs = outIdx0;
	
	// compute Laplacian.
	sum = 0.0;
	idxs = outIdxs;
	incs = inRegion->GetIncrements(); 
	imageExtent = inRegion->GetImageExtent(); 
	for(axisIdx = 0; axisIdx < axesNum; ++axisIdx)
	  {
	  // Compute difference using central differences (if in extent).
	  d = -2.0 * *inPtr0;
	  d += (*idxs == *imageExtent++) ? *inPtr0 : inPtr0[-*incs];
	  d += (*idxs == *imageExtent++) ? *inPtr0 : inPtr0[*incs];
	  sum += d * r[axisIdx]; // divide by aspect ratio squared
	  ++idxs;
	  ++incs;
	  }
	*outPtr0 = sum;

	outPtr0 += outInc0;
	inPtr0 += inInc0;
	}
      outPtr1 += outInc1;
      inPtr1 += inInc1;
      }
    outPtr2 += outInc2;
    inPtr2 += inInc2;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method contains a switch statement that calls the correct
// templated function for the input region type.  The output region
// must be of type float.  This method does handle boundary conditions.
void vtkImageLaplacian::Execute(vtkImageRegion *inRegion, 
				vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  // this filter expects that output is type float.
  if (outRegion->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: output ScalarType, "
        << vtkImageScalarTypeNameMacro(outRegion->GetScalarType())
        << ", must be float");
    return;
    }
  
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageLaplacianExecute(this, inRegion, (float *)(inPtr), 
			       outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageLaplacianExecute(this, inRegion, (int *)(inPtr), 
			       outRegion, (float *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageLaplacianExecute(this, inRegion, (short *)(inPtr), 
			       outRegion, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageLaplacianExecute(this, inRegion, (unsigned short *)(inPtr), 
			       outRegion, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageLaplacianExecute(this, inRegion, (unsigned char *)(inPtr), 
			       outRegion, (float *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}






