/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHarrWavelet2d.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include "vtkImageHarrWavelet2d.h"
#include "vtkImageCache.h"


//----------------------------------------------------------------------------
// Description:
// Constructor: Sets default filter to be identity.
vtkImageHarrWavelet2d::vtkImageHarrWavelet2d()
{
  this->SetAxes2d(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
  this->SetNumberLevels(1);
  this->SetPixelScale(1.0);
  this->SetPixelOffset(0.0);
}


//----------------------------------------------------------------------------
// Description:
// Intercepts the caches UpdateRegion to make the region larger than requested.
// The whole image is generated when any region is requested.
void vtkImageHarrWavelet2d::InterceptCacheUpdate(vtkImageRegion *region)
{
  int bounds[4];

  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Input not set.");
    return;
    }
  
  this->Input->UpdateImageInformation(region);
  region->GetImageBounds2d(bounds);
  region->SetBounds2d(bounds);
}



//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
// Create an addition resolution level.
// Regions bound only the lores. quadrent of the last resolution level.
template <class T>
void vtkImageHarrWavelet2dExecute(vtkImageHarrWavelet2d *self,
				int qSize0, int qSize1,
				vtkImageRegion *inRegion, T *inPtr,
				vtkImageRegion *outRegion, T *outPtr)
{
  T *q0Ptr0, *q0Ptr1;
  T *q1Ptr0, *q1Ptr1;
  T *q2Ptr0, *q2Ptr1;
  T *q3Ptr0, *q3Ptr1;
  int outInc0, outInc1;
  T *inPtr0, *inPtr1;
  int inInc0, inInc1;
  float val0, val1, val2, val3;
  float offset, scale;
  int idx0, idx1;
  
  
  // to make all four quadrent have about the same range
  offset = self->GetPixelOffset();
  scale = self->GetPixelScale();
  
  // Get information to march through data 
  inRegion->GetIncrements2d(inInc0, inInc1);
  outRegion->GetIncrements2d(outInc0, outInc1);
  
  // Loop through ouput quadrent pixels
  inPtr1 = inPtr;
  q0Ptr1 = outPtr;
  q1Ptr1 = outPtr + qSize0 * outInc0;
  q2Ptr1 = outPtr + qSize1 * outInc1;
  q3Ptr1 = q2Ptr1 + qSize0 * outInc0;
  for (idx1 = 0; idx1 < qSize1; ++idx1)
    {
    inPtr0 = inPtr1;
    q0Ptr0 = q0Ptr1;  q1Ptr0 = q1Ptr1;
    q2Ptr0 = q2Ptr1;  q3Ptr0 = q3Ptr1;    
    for (idx0 = 0; idx0 < qSize0; ++idx0)
      {
      val0 = (float)(*inPtr0);
      val1 = (float)(inPtr0[inInc0]);
      val2 = (float)(inPtr0[inInc1]);
      val3 = (float)(inPtr0[inInc0+inInc1]);
      *q0Ptr0 = (T)((val0 + val1 + val2 + val3) / 4.0);
      *q1Ptr0 = (T)(offset + (val0 + val1 - val2 - val3) * scale);
      *q2Ptr0 = (T)(offset + (val0 - val1 + val2 - val3) * scale);
      *q3Ptr0 = (T)(offset + (val0 - val1 - val2 + val3) * scale);
      
      q0Ptr0 += outInc0;  q1Ptr0 += outInc0;
      q2Ptr0 += outInc0;  q3Ptr0 += outInc0;
      inPtr0 += inInc0 * 2;
      }
    q0Ptr1 += outInc1;  q1Ptr1 += outInc1;
    q2Ptr1 += outInc1;  q3Ptr1 += outInc1;
    inPtr1 += inInc1 * 2;
    }
}

    
//----------------------------------------------------------------------------
// Description:
// This method uses the input region to fill the output region.
// It can handle any type data, but the two regions must have the same 
// data type.  Assumes that in and out have the same lower bounds.
void vtkImageHarrWavelet2d::Execute2d(vtkImageRegion *inRegion, 
				    vtkImageRegion *outRegion)
{
  vtkImageRegion *tempRegion = NULL;
  void *inPtr, *outPtr;
  int outMin0, outMax0, outMin1, outMax1;
  int qSize0, qSize1;
  int levelIdx;
  
  
  vtkDebugMacro(<< "Execute2d: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects that input is the same type as output.
  if (inRegion->GetDataType() != outRegion->GetDataType())
    {
    vtkErrorMacro(<< "Execute2d: input DataType, " << inRegion->GetDataType()
                  << ", must match out DataType " << outRegion->GetDataType());
    return;
    }

  // assumes that in and out have the same bounds
  inRegion->GetBounds2d(outMin0, outMax0, outMin1, outMax1);
  qSize0 = (outMax0 - outMin0 + 1);
  qSize1 = (outMax1 - outMin1 + 1);
  
  
  for (levelIdx = 0; levelIdx < this->NumberLevels; ++levelIdx)
    {
    qSize0 /= 2;
    qSize1 /= 2;
    
    inPtr = inRegion->GetVoidPointer2d();
    outPtr = outRegion->GetVoidPointer2d();

    switch (inRegion->GetDataType())
      {
      case VTK_IMAGE_FLOAT:
	vtkImageHarrWavelet2dExecute(this, qSize0, qSize1,
				   inRegion, (float *)(inPtr), 
				   outRegion, (float *)(outPtr));
	break;
      case VTK_IMAGE_INT:
	vtkImageHarrWavelet2dExecute(this, qSize0, qSize1,
				   inRegion, (int *)(inPtr), 
				   outRegion, (int *)(outPtr));
	break;
      case VTK_IMAGE_SHORT:
	vtkImageHarrWavelet2dExecute(this, qSize0, qSize1,
				   inRegion, (short *)(inPtr), 
				   outRegion, (short *)(outPtr));
	break;
      case VTK_IMAGE_UNSIGNED_SHORT:
	vtkImageHarrWavelet2dExecute(this, qSize0, qSize1,
				   inRegion, (unsigned short *)(inPtr), 
				   outRegion, (unsigned short *)(outPtr));
	break;
      case VTK_IMAGE_UNSIGNED_CHAR:
	vtkImageHarrWavelet2dExecute(this, qSize0, qSize1,
				   inRegion, (unsigned char *)(inPtr), 
				   outRegion, (unsigned char *)(outPtr));
	break;
      default:
	vtkErrorMacro(<< "Execute2d: Unknown DataType");
	return;
      }
    
    // If this is not the last iteration, copy output quadrent0 into input
    if (levelIdx+1 < this->NumberLevels)
      {
      // We need a temporary region. (Could use input?)
      outMax0 = outMin0 + qSize0 - 1;
      outMax1 = outMin1 + qSize1 - 1;
      if ( ! tempRegion)
	{
	tempRegion = new vtkImageRegion;
	tempRegion->SetDataType(outRegion->GetDataType());
	// A sore point with me (default coordinates !!!)
	tempRegion->SetBounds(outRegion->GetBounds());
	}
      else
	{
	tempRegion->SetBounds2d(outMin0, outMax0, outMin1, outMax1);
	}
      tempRegion->CopyRegionData(outRegion);
      inRegion = tempRegion;
      }
    }
  
  // Free temporary region
  if (tempRegion)
    {
    tempRegion->Delete();
    }
}
















