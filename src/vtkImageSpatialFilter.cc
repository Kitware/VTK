/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSpatialFilter.cc
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
#include "vtkImageSpatialFilter.hh"


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
  
  this->HandleBoundaries = 1;
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
}






//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the image bounds of this filters
// input, and changes the region to hold the image bounds of this filters
// output.
void vtkImageSpatialFilter::ComputeOutputImageInformation(
		    vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int bounds[8];
  int idx;

  if (this->HandleBoundaries)
    {
    // Output image bounds same as input region bounds
    return;
    }
  
  // shrink output image bounds.
  inRegion->GetImageBounds4d(bounds);
  for (idx = 0; idx < 4; ++idx)
    {
    bounds[idx*2] += this->KernelMiddle[idx];
    bounds[idx*2 + 1] -= (this->KernelSize[idx] - 1) - this->KernelMiddle[idx];
    }
  outRegion->SetBounds4d(bounds);
}





//----------------------------------------------------------------------------
// Description:
// This method computes the bounds of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// bounds of the output region.  After this method finishes, "region" should 
// have the bounds of the required input region.
void vtkImageSpatialFilter::ComputeRequiredInputRegionBounds(
                                                    vtkImageRegion *outRegion, 
			                            vtkImageRegion *inRegion)
{
  int bounds[8];
  int imageBounds[8];
  int idx;
  
  outRegion->GetBounds4d(bounds);
  inRegion->GetImageBounds4d(imageBounds);

  for (idx = 0; idx < 8; ++idx)
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
  inRegion->SetBounds4d(bounds);
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
  int bounds[8];
  int outImageBounds[8];
  int outCenterBounds[8];
  int inBoundsSave[8];
  int outBoundsSave[8];
  
  // Save the bounds of the two regions
  inRegion->GetBounds4d(inBoundsSave);
  outRegion->GetBounds4d(outBoundsSave);
  
  // Compute the image bounds of the output region (no boundary handling/
  inRegion->GetImageBounds4d(outImageBounds);
  for (idx = 0; idx < 4; ++idx)
    {
    outImageBounds[idx*2] += this->KernelMiddle[idx];
    outImageBounds[idx*2+1] -= (this->KernelSize[idx]-1)
      - this->KernelMiddle[idx];
    
    // In case the image is so small, it is all boundary conditions.
    if (outImageBounds[idx*2] > outImageBounds[idx*2+1])
      {
      outImageBounds[idx*2] =(outImageBounds[idx*2]+outImageBounds[idx*2+1])/2;
      outImageBounds[idx*2+1] = outImageBounds[idx*2]-1;
      }
    }

  // Compute the out region that does not need boundary handling.
  outRegion->GetBounds4d(outCenterBounds);
  for (idx = 0; idx < 4; ++idx)
    {
    // Intersection
    if (outCenterBounds[idx*2] < outImageBounds[idx*2])
      {
      outCenterBounds[idx*2] = outImageBounds[idx*2];
      }
    if (outCenterBounds[idx*2+1] > outImageBounds[idx*2+1])
      {
      outCenterBounds[idx*2+1] = outImageBounds[idx*2+1];
      }
    }
  // Call center execute
  outRegion->SetBounds4d(outCenterBounds);
  this->ComputeRequiredInputRegionBounds(outRegion, inRegion);
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
      if (outBoundsSave[idx*2] < outCenterBounds[idx*2])
	{
	for (idx2 = 0; idx2 < 8; ++idx2)
	  {
	  bounds[idx2] = outCenterBounds[idx2];
	  }
	bounds[idx*2] = outBoundsSave[idx*2];
	bounds[idx*2+1] = outCenterBounds[idx*2];
	outRegion->SetBounds4d(bounds);
	this->ComputeRequiredInputRegionBounds(outRegion, inRegion);
	this->ExecuteBoundary4d(inRegion, outRegion);
	outCenterBounds[idx*2] = outBoundsSave[idx*2];
	}
      // for piece right of max
      if (outBoundsSave[idx*2+1] > outCenterBounds[idx*2+1])
	{
	for (idx2 = 0; idx2 < 8; ++idx2)
	  {
	  bounds[idx2] = outCenterBounds[idx2];
	  }
	bounds[idx*2] = outCenterBounds[idx*2+1];
	bounds[idx*2+1] = outBoundsSave[idx*2+1];
	outRegion->SetBounds4d(bounds);
	this->ComputeRequiredInputRegionBounds(outRegion, inRegion);
	this->ExecuteBoundary4d(inRegion, outRegion);
	outCenterBounds[idx*2+1] = outBoundsSave[idx*2+1];
	}
      }
    }
  
  // Restore original bounds just in case
  outRegion->SetBounds4d(outBoundsSave);
  inRegion->SetBounds4d(inBoundsSave);
}




//----------------------------------------------------------------------------
// Description:
// The default execute4d breaks the image in 3d volumes.
void vtkImageSpatialFilter::ExecuteCenter4d(vtkImageRegion *inRegion, 
					    vtkImageRegion *outRegion)
{
  int coordinate3, min3, max3;
  int inBounds[8], outBounds[8];
  
  // Get the bounds of the third dimension to be eliminated.
  inRegion->GetBounds4d(inBounds);
  outRegion->GetBounds4d(outBounds);

  // This method assumes that the third axis of in and out have same bounds.
  min3 = inBounds[6];
  max3 = inBounds[7];
  if (min3 != outBounds[6] || max3 != outBounds[7]) 
    {
    vtkErrorMacro(<< "ExecuteCenter4d: Cannot break 4d images into volumes.");
    return;
    }
  
  // loop over 3d volumes
  for (coordinate3 = min3; coordinate3 <= max3; ++coordinate3)
    {
    // set up the 3d regions.
    inRegion->SetDefaultCoordinate3(coordinate3);
    outRegion->SetDefaultCoordinate3(coordinate3);
    this->ExecuteCenter3d(inRegion, outRegion);
    }
}



//----------------------------------------------------------------------------
// Description:
// The default execute4d breaks the image in 3d volumes.
void vtkImageSpatialFilter::ExecuteBoundary4d(vtkImageRegion *inRegion, 
					      vtkImageRegion *outRegion)
{
  int coordinate3, min3, max3;
  int inBounds[8], outBounds[8];
  
  // Get the bounds of the third dimension to be eliminated.
  inRegion->GetBounds4d(inBounds);
  outRegion->GetBounds4d(outBounds);

  // This method assumes that the third axis of in and out have same bounds.
  min3 = inBounds[6];
  max3 = inBounds[7];
  if (min3 != outBounds[6] || max3 != outBounds[7]) 
    {
    vtkErrorMacro(<< "Executeboundary4d: Cannot break 4d images into volumes.");
    return;
    }
  
  // loop over 3d volumes
  for (coordinate3 = min3; coordinate3 <= max3; ++coordinate3)
    {
    // set up the 3d regions.
    inRegion->SetDefaultCoordinate3(coordinate3);
    outRegion->SetDefaultCoordinate3(coordinate3);
    this->ExecuteBoundary3d(inRegion, outRegion);
    }
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
  int inBounds[6], outBounds[6];
  
  // Get the bounds of the third dimension to be eliminated.
  inRegion->GetBounds3d(inBounds);
  outRegion->GetBounds3d(outBounds);

  // This method assumes that the third axis of in and out have same bounds.
  min2 = inBounds[4];
  max2 = inBounds[5];
  if (min2 != outBounds[4] || max2 != outBounds[5]) 
    {
    vtkErrorMacro(<< "ExecuteCenter3d: Cannot break volumes into images.");
    return;
    }
  
  // loop over 2d images
  for (coordinate2 = min2; coordinate2 <= max2; ++coordinate2)
    {
    // set up the 2d regions.
    inRegion->SetDefaultCoordinate2(coordinate2);
    outRegion->SetDefaultCoordinate2(coordinate2);
    this->ExecuteCenter2d(inRegion, outRegion);
    }
}
  
//----------------------------------------------------------------------------
// Description:
// This method is passed a 3d input and output region, and executes the filter
// algorithm to fill the output from the input.  The default Execute3d
// method breaks the volumes into images.
void vtkImageSpatialFilter::ExecuteBoundary3d(vtkImageRegion *inRegion, 
					      vtkImageRegion *outRegion)
{
  int coordinate2, min2, max2;
  int inBounds[6], outBounds[6];
  
  // Get the bounds of the third dimension to be eliminated.
  inRegion->GetBounds3d(inBounds);
  outRegion->GetBounds3d(outBounds);

  // This method assumes that the third axis of in and out have same bounds.
  min2 = inBounds[4];
  max2 = inBounds[5];
  if (min2 != outBounds[4] || max2 != outBounds[5]) 
    {
    vtkErrorMacro(<< "ExecuteBoundary3d: Cannot break volumes into images.");
    return;
    }
  
  // loop over 2d images
  for (coordinate2 = min2; coordinate2 <= max2; ++coordinate2)
    {
    // set up the 2d regions.
    inRegion->SetDefaultCoordinate2(coordinate2);
    outRegion->SetDefaultCoordinate2(coordinate2);
    this->ExecuteBoundary2d(inRegion, outRegion);
    }
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
  int inBounds[4], outBounds[4];
  
  // Get the bounds of the third dimension to be eliminated.
  inRegion->GetBounds2d(inBounds);
  outRegion->GetBounds2d(outBounds);

  // This method assumes that the second axis of in and out have same bounds.
  min1 = inBounds[2];
  max1 = inBounds[3];
  if (min1 != outBounds[2] || max1 != outBounds[3]) 
    {
    vtkErrorMacro(<< "ExecuteCenter2d: Cannot break images into lines.");
    return;
    }
  
  // loop over 1d lines
  for (coordinate1 = min1; coordinate1 <= max1; ++coordinate1)
    {
    // set up the 1d regions.
    inRegion->SetDefaultCoordinate1(coordinate1);
    outRegion->SetDefaultCoordinate1(coordinate1);
    this->ExecuteCenter1d(inRegion, outRegion);
    }
}
 
//----------------------------------------------------------------------------
// Description:
// This method is passed a 2d input and output region, and executes the filter
// algorithm to fill the output from the input.  The default Execute2d
// method breaks the images into lines.
void vtkImageSpatialFilter::ExecuteBoundary2d(vtkImageRegion *inRegion, 
					      vtkImageRegion *outRegion)
{
  int coordinate1, min1, max1;
  int inBounds[4], outBounds[4];
  
  // Get the bounds of the third dimension to be eliminated.
  inRegion->GetBounds2d(inBounds);
  outRegion->GetBounds2d(outBounds);

  // This method assumes that the second axis of in and out have same bounds.
  min1 = inBounds[2];
  max1 = inBounds[3];
  if (min1 != outBounds[2] || max1 != outBounds[3]) 
    {
    vtkErrorMacro(<< "ExecuteBoundary2d: Cannot break images into lines.");
    return;
    }
  
  // loop over 1d lines
  for (coordinate1 = min1; coordinate1 <= max1; ++coordinate1)
    {
    // set up the 1d regions.
    inRegion->SetDefaultCoordinate1(coordinate1);
    outRegion->SetDefaultCoordinate1(coordinate1);
    this->ExecuteBoundary1d(inRegion, outRegion);
    }
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



//----------------------------------------------------------------------------
// Description:
// This method is passed a 1d input and output region, and executes the filter
// algorithm to fill the output from the input.
void vtkImageSpatialFilter::ExecuteBoundary1d(vtkImageRegion *inRegion, 
					      vtkImageRegion *outRegion)
{
  inRegion = inRegion;
  outRegion = outRegion;
  
  vtkErrorMacro(<< "ExecuteBoundary1d: "
                << "Filter does not specify an ExecuteBoundary method.");
}

