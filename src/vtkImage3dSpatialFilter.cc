/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage3dSpatialFilter.cc
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
#include <math.h>
#include "vtkImage3dSpatialFilter.hh"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImage3dSpatialFilter fitler.
vtkImage3dSpatialFilter::vtkImage3dSpatialFilter()
{
  this->SetKernelSize(0,0,0);
  this->HandleBoundaries = 1;
}


//----------------------------------------------------------------------------
// Description:
// This method sets the size of the 3d neighborhood.  It also sets the 
// default middle of the neighborhood 
void vtkImage3dSpatialFilter::SetKernelSize(int size0, int size1, int size2)
{
  vtkDebugMacro(<< "SetKernelSize: (" << size0 << ", " << size1 << "," 
                << size2);

  this->KernelSize[0] = size0;
  this->KernelSize[1] = size1;
  this->KernelSize[2] = size2;
  this->KernelMiddle[0] = size0 / 2;
  this->KernelMiddle[1] = size1 / 2;
  this->KernelMiddle[2] = size2 / 2;
  this->Modified();
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the image bounds of this filters
// input, and changes the region to hold the image bounds of this filters
// output.
void vtkImage3dSpatialFilter::ComputeOutputImageInformation(
		    vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int bounds[6];
  int idx;

  if (this->HandleBoundaries)
    {
    // Output image bounds same as input region bounds
    return;
    }
  
  // shrink output image bounds.
  inRegion->GetImageBounds3d(bounds);
  for (idx = 0; idx < 3; ++idx)
    {
    bounds[idx*2] += this->KernelMiddle[idx];
    bounds[idx*2 + 1] -= (this->KernelSize[idx] - 1) - this->KernelMiddle[idx];
    }
  outRegion->SetBounds3d(bounds);
}





//----------------------------------------------------------------------------
// Description:
// This method computes the bounds of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// bounds of the output region.  After this method finishes, "region" should 
// have the bounds of the required input region.
void vtkImage3dSpatialFilter::ComputeRequiredInputRegionBounds(
                                                    vtkImageRegion *outRegion, 
			                            vtkImageRegion *inRegion)
{
  int bounds[6];
  int imageBounds[6];
  int idx;
  
  outRegion->GetBounds3d(bounds);
  inRegion->GetImageBounds3d(imageBounds);

  for (idx = 0; idx < 3; ++idx)
    {
    // Expand to get inRegion Bounds
    bounds[idx*2] -= this->KernelMiddle[idx];
    bounds[idx*2 + 1] += (this->KernelSize[idx] - 1) - this->KernelMiddle[idx];

    // If the expanded region is out of the IMAGE Bounds (grow min)
    if (bounds[idx*2] < imageBounds[idx*2])
      {
      if (this->HandleBoundaries)
	{
	// shrink the required region bounds
	bounds[idx*2] = imageBounds[idx*2];
	}
      else
	{
	vtkWarningMacro(<< "Required region is out of the image bounds.");
	}
      }
    // If the expanded region is out of the IMAGE Bounds (shrink max)      
    if (bounds[idx*2+1] > imageBounds[idx*2+1])
      {
      if (this->HandleBoundaries)
	{
	// shrink the required region bounds
	bounds[idx*2+1] = imageBounds[idx*2+1];
	}
      else
	{
	vtkWarningMacro(<< "Required region is out of the image bounds.");
	}
      }
    }
  inRegion->SetBounds3d(bounds);
}















