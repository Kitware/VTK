/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSpatial1d.cxx
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
#include "vtkImageSpatial1d.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageSpatial1d fitler.
vtkImageSpatial1d::vtkImageSpatial1d()
{
  this->KernelSize = 0;
  this->KernelMiddle = 0;
  this->HandleBoundaries = 1;
}


//----------------------------------------------------------------------------
// Description:
// This method sets the width of the 1d neighborhood.  It also sets the 
// default middle of the neighborhood 
void vtkImageSpatial1d::SetKernelSize(int size)
{
  vtkDebugMacro(<< "SetKernelSize: " << ", size = " << size);

  this->KernelSize = size;
  this->KernelMiddle = size / 2;
  this->Modified();
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the boundary of this filters
// input, and changes the region to hold the boundary of this filters
// output.
void vtkImageSpatial1d::ComputeOutputImageInformation(
		    vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int min, max;

  if (this->HandleBoundaries)
    {
    // Output image extent same as input region extent
    return;
    }
  
  // shrink output image extent.
  inRegion->GetImageExtent(min, max);
  min += this->KernelMiddle;
  max -= (this->KernelSize - 1) - this->KernelMiddle;
  outRegion->SetImageExtent(min, max);
}





//----------------------------------------------------------------------------
// Description:
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// extent of the output region.  After this method finishes, "region" should 
// have the extent of the required input region.
void vtkImageSpatial1d::ComputeRequiredInputRegionExtent(
                                                    vtkImageRegion *outRegion, 
			                            vtkImageRegion *inRegion)
{
  int extentMin, extentMax;
  int ImageExtentMin, ImageExtentMax;
  
  outRegion->GetExtent(extentMin, extentMax);
  // Expand to get inRegion Extent
  extentMin -= this->KernelMiddle;
  extentMax += (this->KernelSize - 1) - this->KernelMiddle;

  // If the expanded region is out of the IMAGE Extent
  inRegion->GetImageExtent(ImageExtentMin, ImageExtentMax);
  if (extentMin < ImageExtentMin || extentMax > ImageExtentMax)
    {
    if (this->HandleBoundaries)
      {
      // shrink the required region extent
      extentMin = (extentMin > ImageExtentMin) ? extentMin : ImageExtentMin;
      extentMax = (extentMax < ImageExtentMax) ? extentMax : ImageExtentMax;
      }
    else
      {
      vtkWarningMacro(<< "Required region is out of the image extent.");
      }
    }
  
  inRegion->SetExtent(extentMin, extentMax);
}















