/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSkeleton2D.cxx
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
#include "vtkImageSkeleton2D.h"



//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageSkeleton2D fitler.
vtkImageSkeleton2D::vtkImageSkeleton2D()
{
  this->KernelSize[0] = 3;
  this->KernelSize[1] = 3;
  this->HandleBoundaries = 1;
  this->Prune = 0;
  
  this->SetFilteredAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
}


//----------------------------------------------------------------------------
void vtkImageSkeleton2D::SetFilteredAxes(int axis0, int axis1)
{
  int axes[2];

  axes[0] = axis0;
  axes[1] = axis1;
  this->vtkImageSpatialFilter::SetFilteredAxes(2, axes);
}

//----------------------------------------------------------------------------
// Description:
// This method contains the second switch statement that calls the correct
// templated function for the mask types.
template <class T>
static void vtkImageSkeleton2DExecute(vtkImageSkeleton2D *self,
			   vtkImageRegion *inRegion, T *inPtr, 
			   vtkImageRegion *outRegion, T *outPtr)
{
  // For looping though output (and input) pixels.
  int outMin0, outMax0, outMin1, outMax1;
  int idx0, idx1;
  int inInc0, inInc1;
  int outInc0, outInc1;
  T *inPtr0, *inPtr1;
  T *outPtr0, *outPtr1;
  int wholeMin0, wholeMax0, wholeMin1, wholeMax1;
  int prune = self->GetPrune();
  float n[8];

  //inRegion->MakeDataWritable();
  
  // Get information to march through data
  inRegion->GetIncrements(inInc0, inInc1); 
  outRegion->GetIncrements(outInc0, outInc1); 
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1);
  inRegion->GetExtent(wholeMin0, wholeMax0, wholeMin1, wholeMax1);
  
  // erode input
  inPtr1 = inPtr;
  for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
    {
    inPtr0 = inPtr1;
    for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
      {
      // Center pixel has to be on.
      if (*inPtr0)
	{
	// neighbors independant of boundaries
	n[0] = (idx0>wholeMin0) ? *(inPtr0-inInc0) : 0;
	n[1] = (idx0>wholeMin0)&&(idx1>wholeMin1) 
	  ? *(inPtr0-inInc0-inInc1) : 0;
	n[2] = (idx1>wholeMin1) ? *(inPtr0-inInc1) : 0;
	n[3] = (idx1>wholeMin1)&&(idx0<wholeMax0) 
	  ? *(inPtr0-inInc1+inInc0) : 0;
	n[4] = (idx0<wholeMax0) ? *(inPtr0+inInc0) : 0;
	n[5] = (idx0<wholeMax0)&&(idx1<wholeMax1) 
	  ? *(inPtr0+inInc0+inInc1) : 0;
	n[6] = (idx1<wholeMax1) ? *(inPtr0+inInc1) : 0;
	n[7] = (idx1<wholeMax1)&&(idx0>wholeMin0) 
	  ? *(inPtr0+inInc1-inInc0) : 0;

	// one of four face neighbors has to be off
	if (n[0] == 0 || n[2] == 0 ||
	    n[4] == 0 || n[6] == 0)
	  { // remaining pixels need to be face connected.
	  // across corners
	  if ((n[0] <= 1 || n[2] <= 1 || n[1] > 1) &&
	      (n[2] <= 1 || n[4] <= 1 || n[3] > 1) &&
	      (n[4] <= 1 || n[6] <= 1 || n[5] > 1) &&
	      (n[6] <= 1 || n[0] <= 1 || n[7] > 1))
	    {
	    // opposite faces
	    if ((n[0] <= 1 || n[4] <= 1 || n[2] > 1 || n[6] > 1) &&
		(n[2] <= 1 || n[6] <= 1 || n[0] > 1 || n[4] > 1))
	      {
	      // check for pruning???
	      *inPtr0 = 1;
	      }
	    }
	  }
	}
      
      inPtr0 += inInc0;
      }
    inPtr1 += inInc1;
    }
  // copy to output
  outPtr1 = outPtr;
  inPtr1 = inPtr;
  for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
    {
    outPtr0 = outPtr1;
    inPtr0 = inPtr1;
    for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
      {
      if (*inPtr <= 1)
	{
	*outPtr0 = *inPtr0;
	}
      else
	{
	*outPtr0 = 255;
	}
      *outPtr0 = *inPtr1;
      
      inPtr0 += inInc0;
      outPtr0 += outInc0;      
      }
    inPtr1 += inInc1;
    outPtr1 += outInc1;      
    }
}

//----------------------------------------------------------------------------
// Description:
// This method contains the first switch statement that calls the correct
// templated function for the input and output region types.
void vtkImageSkeleton2D::Execute(vtkImageRegion *inRegion, 
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
      vtkImageSkeleton2DExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageSkeleton2DExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageSkeleton2DExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageSkeleton2DExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageSkeleton2DExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}



  




