/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSpatialFilter.cxx
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
#include "vtkImageSpatialFilter.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageSpatialFilter fitler.
vtkImageSpatialFilter::vtkImageSpatialFilter()
{
  this->KernelSize[0] = 1;
  this->KernelSize[1] = 1;
  this->KernelSize[2] = 1;
  this->KernelSize[3] = 1;

  this->KernelMiddle[0] = 0;
  this->KernelMiddle[1] = 0;
  this->KernelMiddle[2] = 0;
  this->KernelMiddle[3] = 0;
  
  this->HandleBoundariesOn();
  this->UseExecuteCenterOn();
}


//----------------------------------------------------------------------------
void vtkImageSpatialFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFilter::PrintSelf(os, indent);
  os << indent << "KernelSize: (" << this->KernelSize[0]
     << ", " << this->KernelSize[1] << ", " << this->KernelSize[2]
     << ", " << this->KernelSize[3] << ").\n";
  os << indent << "KernelMiddle: (" << this->KernelMiddle[0]
     << ", " << this->KernelMiddle[1] << ", " << this->KernelMiddle[2]
     << ", " << this->KernelMiddle[3] << ").\n";
  os << indent << "UseExecuteCenter: " << this->UseExecuteCenter << "\n";
}






//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the image extent of this filters
// input, and changes the region to hold the image extent of this filters
// output.
void vtkImageSpatialFilter::ComputeOutputImageInformation(
		    vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int extent[8];
  int idx;

  if (this->HandleBoundaries)
    {
    // Output image extent same as input region extent
    return;
    }
  
  // shrink output image extent.
  inRegion->GetImageExtent(extent, 4);
  for (idx = 0; idx < 4; ++idx)
    {
    extent[idx*2] += this->KernelMiddle[idx];
    extent[idx*2 + 1] -= (this->KernelSize[idx] - 1) - this->KernelMiddle[idx];
    }
  outRegion->SetImageExtent(extent, 4);
}





//----------------------------------------------------------------------------
// Description:
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// extent of the output region.  After this method finishes, "region" should 
// have the extent of the required input region.
void vtkImageSpatialFilter::ComputeRequiredInputRegionExtent(
                                                    vtkImageRegion *outRegion, 
			                            vtkImageRegion *inRegion)
{
  int extent[8];
  int imageExtent[8];
  int idx;
  
  outRegion->GetExtent(extent, 4);
  inRegion->GetImageExtent(imageExtent, 4);

  for (idx = 0; idx < 4; ++idx)
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
  inRegion->SetExtent(extent, 4);
}



//----------------------------------------------------------------------------
// Description:
// This Execute method breaks the regions into pieces that have boundaries
// and a piece that does not need boundary handling.  It calls subclass
// defined execute methods for these pieces.
void vtkImageSpatialFilter::Execute4d(vtkImageRegion *inRegion,
				      vtkImageRegion *outRegion)
{
  int idx, idx2;
  int extent[8];
  int outImageExtent[8];
  int outCenterExtent[8];
  int inExtentSave[8];
  int outExtentSave[8];
  
  // If a separate center method does not exist, don't bother splitting
  if ( ! this->UseExecuteCenter)
    {
    this->vtkImageFilter::Execute4d(inRegion, outRegion);
    return;
    }
  
  // Save the extent of the two regions
  inRegion->GetExtent(inExtentSave, 4);
  outRegion->GetExtent(outExtentSave, 4);
  
  // Compute the image extent of the output region (no boundary handling)
  inRegion->GetImageExtent(outImageExtent, 4);
  for (idx = 0; idx < 4; ++idx)
    {
    outImageExtent[idx*2] += this->KernelMiddle[idx];
    outImageExtent[idx*2+1] -= (this->KernelSize[idx]-1)
      - this->KernelMiddle[idx];
    
    // In case the image is so small, it is all boundary conditions.
    if (outImageExtent[idx*2] > outImageExtent[idx*2+1])
      {
      outImageExtent[idx*2] =(outImageExtent[idx*2]+outImageExtent[idx*2+1])/2;
      outImageExtent[idx*2+1] = outImageExtent[idx*2]-1;
      }
    }

  // Compute the out region that does not need boundary handling.
  outRegion->GetExtent(outCenterExtent, 4);
  for (idx = 0; idx < 4; ++idx)
    {
    // Intersection
    if (outCenterExtent[idx*2] < outImageExtent[idx*2])
      {
      outCenterExtent[idx*2] = outImageExtent[idx*2];
      }
    if (outCenterExtent[idx*2+1] > outImageExtent[idx*2+1])
      {
      outCenterExtent[idx*2+1] = outImageExtent[idx*2+1];
      }
    }
  // Call center execute
  outRegion->SetExtent(outCenterExtent, 4);
  this->ComputeRequiredInputRegionExtent(outRegion, inRegion);
  // Just in cass the image is so small there is no center.
  if (outRegion->GetVolume() > 0)
    {
    this->ExecuteCenter4d(inRegion, outRegion);
    }
  
  // Do stuff for all boundary pieces
  if (this->HandleBoundaries)
    {
    // start getting and executing boundary pieces.
    for (idx = 0; idx < 4; ++idx)
      {
      // for piece left of min
      if (outExtentSave[idx*2] < outCenterExtent[idx*2])
	{
	for (idx2 = 0; idx2 < 8; ++idx2)
	  {
	  extent[idx2] = outCenterExtent[idx2];
	  }
	extent[idx*2] = outExtentSave[idx*2];
	extent[idx*2+1] = outCenterExtent[idx*2];
	outRegion->SetExtent(extent, 4);
	this->ComputeRequiredInputRegionExtent(outRegion, inRegion);
	this->vtkImageFilter::Execute4d(inRegion, outRegion);
	outCenterExtent[idx*2] = outExtentSave[idx*2];
	}
      // for piece right of max
      if (outExtentSave[idx*2+1] > outCenterExtent[idx*2+1])
	{
	for (idx2 = 0; idx2 < 8; ++idx2)
	  {
	  extent[idx2] = outCenterExtent[idx2];
	  }
	extent[idx*2] = outCenterExtent[idx*2+1];
	extent[idx*2+1] = outExtentSave[idx*2+1];
	outRegion->SetExtent(extent, 4);
	this->ComputeRequiredInputRegionExtent(outRegion, inRegion);
	this->vtkImageFilter::Execute4d(inRegion, outRegion);
	outCenterExtent[idx*2+1] = outExtentSave[idx*2+1];
	}
      }
    }
  
  // Restore original extent just in case
  outRegion->SetExtent(outExtentSave, 4);
  inRegion->SetExtent(inExtentSave, 4);
}




//----------------------------------------------------------------------------
// Description:
// The default execute4d breaks the image in 3d volumes.
void vtkImageSpatialFilter::ExecuteCenter4d(vtkImageRegion *inRegion, 
					    vtkImageRegion *outRegion)
{
  int coordinate3, min3, max3;
  int inExtent[8], outExtent[8];
  
  // Get the extent of the third dimension to be eliminated.
  inRegion->GetExtent(inExtent, 4);
  outRegion->GetExtent(outExtent, 4);

  // This method assumes that the third axis of in and out have same extent.
  min3 = inExtent[6];
  max3 = inExtent[7];
  if (min3 != outExtent[6] || max3 != outExtent[7]) 
    {
    vtkErrorMacro(<< "ExecuteCenter4d: Cannot break 4d images into volumes.");
    return;
    }
  
  // loop over 3d volumes
  for (coordinate3 = min3; coordinate3 <= max3; ++coordinate3)
    {
    // set up the 3d regions.
    inExtent[6] = coordinate3;
    inExtent[7] = coordinate3;
    inRegion->SetExtent(inExtent, 4);
    outExtent[6] = coordinate3;
    outExtent[7] = coordinate3;
    outRegion->SetExtent(outExtent, 4);
    this->ExecuteCenter3d(inRegion, outRegion);
    }
  // restore the original extent
  inExtent[6] = min3;
  inExtent[7] = max3;
  outExtent[6] = min3;
  outExtent[7] = max3; 
  inRegion->SetExtent(inExtent, 4);
  outRegion->SetExtent(outExtent, 4);
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a 3d input and output region, and executes the filter
// algorithm to fill the output from the input.  The default Execute3d
// method breaks the volumes into images.
void vtkImageSpatialFilter::ExecuteCenter3d(vtkImageRegion *inRegion, 
					    vtkImageRegion *outRegion)
{
  int coordinate2, min2, max2;
  int inExtent[6], outExtent[6];
  
  // Get the extent of the third dimension to be eliminated.
  inRegion->GetExtent(inExtent, 3);
  outRegion->GetExtent(outExtent, 3);

  // This method assumes that the third axis of in and out have same extent.
  min2 = inExtent[4];
  max2 = inExtent[5];
  if (min2 != outExtent[4] || max2 != outExtent[5]) 
    {
    vtkErrorMacro(<< "ExecuteCenter3d: Cannot break volumes into images.");
    return;
    }
  
  // loop over 2d images
  for (coordinate2 = min2; coordinate2 <= max2; ++coordinate2)
    {
    // set up the 2d regions.
    inExtent[4] = coordinate2;
    inExtent[5] = coordinate2;
    inRegion->SetExtent(inExtent, 3);
    outExtent[4] = coordinate2;
    outExtent[5] = coordinate2;
    outRegion->SetExtent(outExtent, 3);
    this->ExecuteCenter2d(inRegion, outRegion);
    }
  // restore the original extent
  inExtent[4] = min2;
  inExtent[5] = max2;
  outExtent[4] = min2;
  outExtent[5] = max2; 
  inRegion->SetExtent(inExtent, 3);
  outRegion->SetExtent(outExtent, 3);
}
  
//----------------------------------------------------------------------------
// Description:
// This method is passed a 2d input and output region, and executes the filter
// algorithm to fill the output from the input.  The default Execute2d
// method breaks the images into lines.  
void vtkImageSpatialFilter::ExecuteCenter2d(vtkImageRegion *inRegion, 
					    vtkImageRegion *outRegion)
{
  int coordinate1, min1, max1;
  int inExtent[4], outExtent[4];
  
  // Get the extent of the third dimension to be eliminated.
  inRegion->GetExtent(inExtent, 2);
  outRegion->GetExtent(outExtent, 2);

  // This method assumes that the second axis of in and out have same extent.
  min1 = inExtent[2];
  max1 = inExtent[3];
  if (min1 != outExtent[2] || max1 != outExtent[3]) 
    {
    vtkErrorMacro(<< "ExecuteCenter2d: Cannot break images into lines.");
    return;
    }
  
  // loop over 1d lines
  for (coordinate1 = min1; coordinate1 <= max1; ++coordinate1)
    {
    // set up the 1d regions.
    inExtent[2] = coordinate1;
    inExtent[3] = coordinate1;
    inRegion->SetExtent(inExtent, 2);
    outExtent[2] = coordinate1;
    outExtent[3] = coordinate1;
    outRegion->SetExtent(outExtent, 2);
    this->ExecuteCenter1d(inRegion, outRegion);
    }
  // restore the original extent
  inExtent[2] = min1;
  inExtent[3] = max1;
  outExtent[2] = min1;
  outExtent[3] = max1; 
  inRegion->SetExtent(inExtent, 2);
  outRegion->SetExtent(outExtent, 2);
}
 
//----------------------------------------------------------------------------
// Description:
// This method is passed a 1d input and output region, and executes the filter
// algorithm to fill the output from the input.  
void vtkImageSpatialFilter::ExecuteCenter1d(vtkImageRegion *inRegion, 
					    vtkImageRegion *outRegion)
{
  inRegion = inRegion;
  outRegion = outRegion;
  
  vtkErrorMacro(<< "ExecuteCenter1d: " 
                << "Filter does not specify an ExecuteCenter method.");
}



