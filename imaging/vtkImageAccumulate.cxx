/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAccumulate.cxx
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
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageAccumulate.h"
#include <math.h>
#include <stdlib.h>


//----------------------------------------------------------------------------
// Description:
// Constructor sets default values
vtkImageAccumulate::vtkImageAccumulate()
{
  int idx, axes[5];
  
  for (idx = 0; idx < 4; ++idx)
    {
    this->ComponentSpacing[idx] = 1.0;
    this->ComponentOrigin[idx] = 0.0;
    this->ComponentExtent[idx*2] = 0;
    this->ComponentExtent[idx*2+1] = 0;
    }
  this->ComponentExtent[1] = 255;
  
  axes[0] = VTK_IMAGE_X_AXIS;
  axes[1] = VTK_IMAGE_Y_AXIS;
  axes[2] = VTK_IMAGE_Z_AXIS;
  axes[3] = VTK_IMAGE_TIME_AXIS;
  axes[4] = VTK_IMAGE_COMPONENT_AXIS;
  
  this->SetExecutionAxes(5, axes);
  
  this->SetOutputScalarTypeToInt();
}

//----------------------------------------------------------------------------
void vtkImageAccumulate::SetComponentSpacing(int dim, float *spacing)
{
  int idx;
  
  if (dim > 4)
    {
    vtkWarningMacro("Dimensions too large");
    dim = 4;
    }
  
  for (idx = 0; idx < dim; ++idx)
    {
    this->ComponentSpacing[idx] = spacing[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageAccumulate::SetComponentOrigin(int dim, float *origin)
{
  int idx;
  
  if (dim > 4)
    {
    vtkWarningMacro("Dimensions too large");
    dim = 4;
    }
  
  for (idx = 0; idx < dim; ++idx)
    {
    this->ComponentOrigin[idx] = origin[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageAccumulate::SetComponentExtent(int dim, int *extent)
{
  int idx;
  
  if (dim > 4)
    {
    vtkWarningMacro("Dimensions too large");
    dim = 4;
    }
  
  for (idx = 0; idx < dim; ++idx)
    {
    this->ComponentExtent[idx*2] = extent[idx*2];
    this->ComponentExtent[idx*2+1] = extent[idx*2+1];
    }
}




//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageAccumulateExecute(vtkImageAccumulate *self,
				      vtkImageRegion *inRegion, T *inPtr,
				      vtkImageRegion *outRegion, int *outPtr)
{
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int idx0, idx1, idx2, idx3, idxC;
  int inInc0, inInc1, inInc2, inInc3, inIncC;
  T *inPtr0, *inPtr1, *inPtr2, *inPtr3, *inPtrC;
  int *outPtrC;
  int numC, outIdx, *outExtent, *outIncs;
  float *origin, *spacing;

  self = self;
  // Zero count in every bin
  outRegion->Fill(0.0);
  
  // Get information to march through data 
  inRegion->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min0, max0);
  numC = max0 - min0 + 1;
  inRegion->GetExtent(min0, max0, min1, max1, min2, max2, min3, max3);
  inRegion->GetIncrements(inInc0, inInc1, inInc2, inInc3, inIncC);
  outExtent = outRegion->GetExtent();
  outIncs = outRegion->GetIncrements();
  origin = outRegion->GetOrigin();
  spacing = outRegion->GetSpacing();
  
  inPtr3 = inPtr;
  for (idx3 = min3; idx3 <= max3; ++idx3)
    {
    inPtr2 = inPtr3;
    for (idx2 = min2; idx2 <= max2; ++idx2)
      {
      inPtr1 = inPtr2;
      for (idx1 = min1; idx1 <= max1; ++idx1)
	{
	inPtr0  = inPtr1;
	for (idx0 = min0; idx0 <= max0; ++idx0)
	  {
	  inPtrC = inPtr0;
	  // find the bin for this pixel.
	  outPtrC = outPtr;
	  for (idxC = 0; idxC < numC; ++idxC)
	    {
	    // compute the index
	    outIdx = (int)((*inPtrC - origin[idxC]) / spacing[idxC]);
	    if (outIdx < outExtent[idxC*2] || outIdx > outExtent[idxC*2+1])
	      {
	      // Out of bin range
	      outPtrC = NULL;
	      break;
	      }
	    outPtrC += outIdx * outIncs[idxC];
	    inPtrC += inIncC;
	    }
	  if (outPtrC)
	    {
	    ++(*outPtrC);
	    }

	  inPtr0 += inInc0;
	  }
	inPtr1 += inInc1;
	}
      inPtr2 += inInc2;
      }
    inPtr3 += inInc3;
    }
}

	

//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageAccumulate::Execute(vtkImageRegion *inRegion, 
				  vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  // this filter expects that output is type int.
  if (outRegion->GetScalarType() != VTK_INT)
    {
    vtkErrorMacro(<< "Execute: out ScalarType " << outRegion->GetScalarType()
		  << " must be int\n");
    return;
    }
  
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageAccumulateExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_INT:
      vtkImageAccumulateExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageAccumulateExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageAccumulateExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageAccumulateExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the boundary of this filters
// input, and changes the region to hold the boundary of this filters
// output.
void vtkImageAccumulate::ExecuteImageInformation()
{
  this->Output->SetWholeExtent(this->ComponentExtent);
  this->Output->SetOrigin(this->ComponentOrigin);
  this->Output->SetSpacing(this->ComponentSpacing);
  this->Output->SetNumberOfScalarComponents(1);
}

//----------------------------------------------------------------------------
// Description:
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// extent of the output region.  After this method finishes, "region" should 
// have the extent of the required input region.
void vtkImageAccumulate::ComputeRequiredInputUpdateExtent()
{
  int wholeExtent[8];
  
  this->Input->GetWholeExtent(wholeExtent);
  this->Input->SetUpdateExtent(wholeExtent);
}

//----------------------------------------------------------------------------
// Description:
// Intercepts the caches Update to make the region larger than requested.
// We might as well create both real and imaginary components.
void vtkImageAccumulate::InterceptCacheUpdate()
{
  int wholeExtent[8];
  
  // Filter superclass has no control of intercept cache update.
  // a work around
  if (this->Bypass)
    {
    return;
    }
  
  this->Output->GetWholeExtent(wholeExtent);
  this->Output->SetUpdateExtent(wholeExtent);
}





