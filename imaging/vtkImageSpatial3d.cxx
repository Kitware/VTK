/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSpatial3d.cxx
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
#include "vtkImageSpatial3d.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageSpatial3d fitler.
vtkImageSpatial3d::vtkImageSpatial3d()
{
  this->SetKernelSize(0,0,0);
  this->HandleBoundaries = 1;
}


//----------------------------------------------------------------------------
// Description:
// This method sets the size of the 3d neighborhood.  It also sets the 
// default middle of the neighborhood 
void vtkImageSpatial3d::SetKernelSize(int size0, int size1, int size2)
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
// This method is passed a region that holds the image extent of this filters
// input, and changes the region to hold the image extent of this filters
// output.
void vtkImageSpatial3d::ComputeOutputImageInformation(
		    vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int extent[6];
  int idx;

  if (this->HandleBoundaries)
    {
    // Output image extent same as input region extent
    return;
    }
  
  // shrink output image extent.
  inRegion->GetImageExtent(extent, 3);
  for (idx = 0; idx < 3; ++idx)
    {
    extent[idx*2] += this->KernelMiddle[idx];
    extent[idx*2 + 1] -= (this->KernelSize[idx] - 1) - this->KernelMiddle[idx];
    }
  outRegion->SetExtent(extent, 3);
}





//----------------------------------------------------------------------------
// Description:
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// extent of the output region.  After this method finishes, "region" should 
// have the extent of the required input region.
void vtkImageSpatial3d::ComputeRequiredInputRegionExtent(
                                                    vtkImageRegion *outRegion, 
			                            vtkImageRegion *inRegion)
{
  int extent[6];
  int imageExtent[6];
  int idx;
  
  outRegion->GetExtent(extent, 3);
  inRegion->GetImageExtent(imageExtent, 3);

  for (idx = 0; idx < 3; ++idx)
    {
    // Expand to get inRegion Extent
    extent[idx*2] -= this->KernelMiddle[idx];
    extent[idx*2 + 1] += (this->KernelSize[idx] - 1) - this->KernelMiddle[idx];

    // If the expanded region is out of the IMAGE Extent (grow min)
    if (extent[idx*2] < imageExtent[idx*2])
      {
      if (this->HandleBoundaries)
	{
	// shrink the required region extent
	extent[idx*2] = imageExtent[idx*2];
	}
      else
	{
	vtkWarningMacro(<< "Required region is out of the image extent.");
	}
      }
    // If the expanded region is out of the IMAGE Extent (shrink max)      
    if (extent[idx*2+1] > imageExtent[idx*2+1])
      {
      if (this->HandleBoundaries)
	{
	// shrink the required region extent
	extent[idx*2+1] = imageExtent[idx*2+1];
	}
      else
	{
	vtkWarningMacro(<< "Required region is out of the image extent.");
	}
      }
    }
  inRegion->SetExtent(extent, 3);
}















