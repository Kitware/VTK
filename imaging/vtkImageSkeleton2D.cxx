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


// 0 => remove pixel
// 1 => remove if prunning
// 2 => do not remove
// 4 8 3
// 5   7
// 1 6 2
static unsigned char VTK_IMAGE_SKELETON2D_CASES[] = 
{
  1, 1, 1, 2,   1, 2, 2, 2,    // 0-7
  1, 2, 2, 2,   2, 2, 2, 2,    // 8-15
  1, 0, 2, 0,   2, 0, 2, 0,    // 16-23
  0, 0, 0, 0,   0, 0, 0, 0,    // 24-31
  1, 0, 0, 0,   2, 0, 0, 0,    // 32-39
  2, 0, 0, 0,   2, 0, 0, 0,    // 40-47
  0, 0, 0, 0,   0, 0, 0, 0,    // 48-55
  0, 0, 0, 0,   0, 0, 0, 0,    // 56-63
  1, 2, 0, 0,   0, 0, 0, 0,    // 64-71
  2, 2, 0, 0,   0, 0, 0, 0,    // 72-79
  2, 0, 0, 0,   0, 0, 0, 0,    // 80-87
  0, 0, 0, 0,   0, 0, 0, 0,    // 88-95
  0, 0, 0, 0,   0, 0, 0, 0,    // 99-103
  0, 0, 0, 0,   0, 0, 0, 0,    // 104-111
  0, 0, 0, 0,   0, 0, 0, 0,    // 112-119
  0, 0, 0, 0,   0, 0, 0, 1,    // 120-127

  1, 2, 2, 2,   0, 0, 0, 0,    // 128-135
  0, 0, 0, 0,   0, 0, 0, 0,    // 136-143
  0, 0, 0, 0,   0, 0, 0, 0,    // 144-151
  0, 0, 0, 0,   0, 0, 0, 0,    // 152-159
  2, 0, 0, 0,   0, 0, 0, 0,    // 160-167
  0, 0, 0, 0,   0, 0, 0, 0,    // 168-175
  0, 0, 0, 0,   0, 0, 0, 0,    // 176-183
  0, 0, 0, 0,   0, 0, 0, 0,    // 184-191
  0, 0, 0, 0,   0, 0, 0, 0,    // 192-199
  0, 0, 0, 0,   0, 0, 0, 0,    // 200-207
  0, 0, 0, 0,   0, 0, 0, 0,    // 208-215
  0, 0, 0, 0,   0, 0, 0, 0,    // 216-223
  0, 0, 0, 0,   0, 0, 0, 0,    // 224-231
  0, 0, 0, 0,   0, 0, 0, 0,    // 232-239
  2, 2, 2, 0,   2, 0, 0, 0,    // 240-247
  2, 0, 0, 0,   0, 0, 0, 2     // 248-255
};

  
  


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
  int number, prune = self->GetPrune();
  
  // Get information to march through data
  inRegion->GetIncrements(inInc0, inInc1); 
  outRegion->GetIncrements(outInc0, outInc1); 
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1);
  inRegion->GetExtent(wholeMin0, wholeMax0, wholeMin1, wholeMax1);
  
  // loop through pixel of output
  outPtr1 = outPtr;
  inPtr1 = inPtr;
  for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
    {
    outPtr0 = outPtr1;
    inPtr0 = inPtr1;
    for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
      {
      *outPtr0 = *inPtr0;
      
      if (*inPtr0)
	{
	number = 0;
	// corners
	if (idx0 > wholeMin0 && idx1 > wholeMin1 && *(inPtr0-inInc0-inInc1))
	  {
	  number += 1;
	  }
	if (idx0 < wholeMax0 && idx1 > wholeMin1 && *(inPtr0+inInc0-inInc1))
	  {
	  number += 2;
	  }
	if (idx0 < wholeMax0 && idx1 < wholeMax1 && *(inPtr0+inInc0+inInc1))
	  {
	  number += 4;
	  }
	if (idx0 > wholeMin0 && idx1 < wholeMax1 && *(inPtr0-inInc0+inInc1))
	  {
	  number += 8;
	  }
	// edges
	if (idx0 > wholeMin0 && *(inPtr0-inInc0))
	  {
	  number += 16;
	  }
	if (idx1 > wholeMin1 && *(inPtr0-inInc1))
	  {
	  number += 32;
	  }
	if (idx0 < wholeMax0 && *(inPtr0+inInc0))
	  {
	  number += 64;
	  }
	if (idx1 < wholeMax1 && *(inPtr0+inInc1))
	  {
	  number += 128;
	  }
	// set output pixel;
	if (VTK_IMAGE_SKELETON2D_CASES[number] == 0)
	  {
	  *outPtr0 = (T)(0);
	  }
	if (prune && VTK_IMAGE_SKELETON2D_CASES[number] == 1)
	  {
	  *outPtr0 = (T)(0);
	  }
	}
      
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



  




