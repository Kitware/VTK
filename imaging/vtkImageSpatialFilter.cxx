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
  int idx;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->KernelSize[idx] = 1;
    this->KernelMiddle[idx] = 0;
    this->Strides[idx] = 1;
    }
  
  this->HandleBoundaries = 1;
  this->UseExecuteCenter = 1;
}


//----------------------------------------------------------------------------
void vtkImageSpatialFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkImageFilter::PrintSelf(os, indent);

  os << indent << "KernelSize: (" << this->KernelSize[0];
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << this->KernelSize[idx];
    }
  os << ").\n";

  os << indent << "KernelMiddle: (" << this->KernelMiddle[0];
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << this->KernelMiddle[idx];
    }
  os << ").\n";

  os << indent << "Strides: (" << this->Strides[0];
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << this->Strides[idx];
    }
  os << ").\n";

  os << indent << "UseExecuteCenter: " << this->UseExecuteCenter << "\n";
}


//----------------------------------------------------------------------------
void vtkImageSpatialFilter::GetKernelSize(int num, int *size)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    size[idx] = this->KernelSize[idx];
    }
}



//----------------------------------------------------------------------------
void vtkImageSpatialFilter::GetKernelMiddle(int num, int *middle)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    middle[idx] = this->KernelMiddle[idx];
    }
}



//----------------------------------------------------------------------------
void vtkImageSpatialFilter::GetStrides(int num, int *strides)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    strides[idx] = this->Strides[idx];
    }
}




//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the image extent of this filters
// input, and changes the region to hold the image extent of this filters
// output.
void vtkImageSpatialFilter::ComputeOutputImageInformation(
		    vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  float aspectRatio[VTK_IMAGE_DIMENSIONS];
  int idx;
  
  inRegion->GetImageExtent(VTK_IMAGE_DIMENSIONS, extent);
  inRegion->GetAspectRatio(VTK_IMAGE_DIMENSIONS, aspectRatio);

  this->ComputeOutputImageExtent(extent, this->HandleBoundaries);
  outRegion->SetImageExtent(VTK_IMAGE_DIMENSIONS, extent);
  
  for(idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    // Change the aspect ratio.
    aspectRatio[idx] *= (float)(this->Strides[idx]);
    }
  outRegion->SetAspectRatio(VTK_IMAGE_DIMENSIONS, aspectRatio);
}

//----------------------------------------------------------------------------
// Description:
// A helper method to compute output image extent
void vtkImageSpatialFilter::ComputeOutputImageExtent(int *extent, 
						     int handleBoundaries)
{
  int idx;

  if ( ! handleBoundaries)
    {
    // Make extent a little smaller because of the kernel size.
    for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
      {
      extent[idx*2] += this->KernelMiddle[idx];
      extent[idx*2+1] -= (this->KernelSize[idx] - 1) - this->KernelMiddle[idx];
      }
    }
  
  for(idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    // Scale the output extent because of strides
    extent[idx*2] = 
      (int)(ceil(((float)extent[idx*2]) /((float)this->Strides[idx])));
    extent[idx*2+1] = 
      (int)(floor((((float)extent[idx*2+1]+1.0) /
		   ((float)this->Strides[idx]))-1.0));
    // Change the aspect ratio.
    }
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
  int extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int imageExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int idx;
  
  outRegion->GetExtent(VTK_IMAGE_DIMENSIONS, extent);
  inRegion->GetImageExtent(VTK_IMAGE_DIMENSIONS, imageExtent);

  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    // Magnify by strides
    extent[idx*2] *= this->Strides[idx];
    extent[idx*2+1] = (extent[idx*2+1]+1)*this->Strides[idx] - 1;
    // Expand to get inRegion Extent
    extent[idx*2] -= this->KernelMiddle[idx];
    extent[idx*2+1] += (this->KernelSize[idx] - 1) - this->KernelMiddle[idx];

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
  inRegion->SetExtent(VTK_IMAGE_DIMENSIONS, extent);
}


//----------------------------------------------------------------------------
// Description:
// This Execute method breaks the regions into pieces that have boundaries
// and a piece that does not need boundary handling.  It calls subclass
// defined execute methods for these pieces.
void vtkImageSpatialFilter::Execute(int dim, vtkImageRegion *inRegion,
				    vtkImageRegion *outRegion)
{
  int idx, idx2;
  int *extent = new int[dim*2];
  int *outImageExtent = new int[dim*2];
  int *outCenterExtent = new int[dim*2];
  int *inExtentSave = new int[dim*2];
  int *outExtentSave = new int[dim*2];
  
  // If a separate center method does not exist, don't bother splitting
  if ( ! this->UseExecuteCenter)
    {
    this->vtkImageFilter::Execute(dim, inRegion, outRegion);
    delete [] extent;
    delete [] outImageExtent;
    delete [] outCenterExtent;
    delete [] inExtentSave;
    delete [] outExtentSave;    
    return;
    }

  // Save the extent of the two regions
  inRegion->GetExtent(dim, inExtentSave);
  outRegion->GetExtent(dim, outExtentSave);
  
  // Compute the image extent of the output region (no boundary handling)
  inRegion->GetImageExtent(dim, outImageExtent);
  this->ComputeOutputImageExtent(outImageExtent, 0);

  // Intersect with the actual output extent.
  outRegion->GetExtent(dim, outCenterExtent);
  for (idx = 0; idx < dim; ++idx)
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
  outRegion->SetExtent(dim, outCenterExtent);
  this->ComputeRequiredInputRegionExtent(outRegion, inRegion);
  // Just in cass the image is so small there is no center.
  if (outRegion->GetVolume() > 0)
    {
    this->ExecuteCenter(dim, inRegion, outRegion);
    }
  
  // Do stuff for all boundary pieces
  if (this->HandleBoundaries)
    {
    // start getting and executing boundary pieces.
    for (idx = 0; idx < dim; ++idx)
      {
      // for piece left of min
      if (outExtentSave[idx*2] < outCenterExtent[idx*2])
	{
	for (idx2 = 0; idx2 < dim*2; ++idx2)
	  {
	  extent[idx2] = outCenterExtent[idx2];
	  }
	extent[idx*2] = outExtentSave[idx*2];
	extent[idx*2+1] = outCenterExtent[idx*2];
	outRegion->SetExtent(dim, extent);
	this->ComputeRequiredInputRegionExtent(outRegion, inRegion);
	this->vtkImageFilter::Execute(dim, inRegion, outRegion);
	outCenterExtent[idx*2] = outExtentSave[idx*2];
	}
      // for piece right of max
      if (outExtentSave[idx*2+1] > outCenterExtent[idx*2+1])
	{
	for (idx2 = 0; idx2 < dim*2; ++idx2)
	  {
	  extent[idx2] = outCenterExtent[idx2];
	  }
	extent[idx*2] = outCenterExtent[idx*2+1];
	extent[idx*2+1] = outExtentSave[idx*2+1];
	outRegion->SetExtent(dim, extent);
	this->ComputeRequiredInputRegionExtent(outRegion, inRegion);
	this->vtkImageFilter::Execute(dim, inRegion, outRegion);
	outCenterExtent[idx*2+1] = outExtentSave[idx*2+1];
	}
      }
    }
  
  // Restore original extent just in case
  outRegion->SetExtent(dim, outExtentSave);
  inRegion->SetExtent(dim, inExtentSave);
  delete [] extent;
  delete [] outImageExtent;
  delete [] outCenterExtent;
  delete [] inExtentSave;
  delete [] outExtentSave;    
}




//----------------------------------------------------------------------------
// Description:
// The default execute4d breaks the image in 3d volumes.
void vtkImageSpatialFilter::ExecuteCenter(int dim,
					  vtkImageRegion *inRegion, 
					  vtkImageRegion *outRegion)
{
  int coordinate, axis;
  int inMin, inMax;
  int outMin, outMax;

  
  // Terminate recursion?
  if (dim <= this->Dimensionality)
    {
    this->ExecuteCenter(inRegion, outRegion);
    return;
    }
  
  // Get the extent of the third dimension to be eliminated.
  axis = this->Axes[dim - 1];
  inRegion->GetAxisExtent(axis, inMin, inMax);
  outRegion->GetAxisExtent(axis, outMin, outMax);

  // This method assumes that axis to be eliminated have the same extent.
  if (inMin != outMin || inMax != outMax) 
    {
    vtkErrorMacro(<< "ExecuteCenter: Extent mismatch.");
    return;
    }
  
  // loop over extra axis
  for (coordinate = inMin; coordinate <= inMax; ++coordinate)
    {
    // set up the lower dimensional regions.
    inRegion->SetAxisExtent(axis, coordinate, coordinate);
    outRegion->SetAxisExtent(axis, coordinate, coordinate);
    this->vtkImageSpatialFilter::ExecuteCenter(dim - 1, inRegion, outRegion);
    }
  // restore the original extent
  inRegion->SetAxisExtent(axis, inMin, inMax);
  outRegion->SetAxisExtent(axis, outMin, outMax);
}



//----------------------------------------------------------------------------
// Description:
// Subclass must provide this function if UseExecuteCenter is on.
void vtkImageSpatialFilter::ExecuteCenter(vtkImageRegion *inRegion, 
					  vtkImageRegion *outRegion)
{
  inRegion = outRegion;
  vtkErrorMacro(<< "Subclass does not have an ExecuteCenter method.");
}
  
