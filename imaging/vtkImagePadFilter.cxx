/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePadFilter.cxx
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
#include "vtkImagePadFilter.h"



//----------------------------------------------------------------------------
// Description:
// Constructor sets default values
vtkImagePadFilter::vtkImagePadFilter()
{
  int idx;
  
  // Initialize output image extent to one pixel (origin)
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->OutputImageExtent[idx * 2] = 0;
    this->OutputImageExtent[idx * 2 + 1] = 0;
    }
}



//----------------------------------------------------------------------------
void vtkImagePadFilter::SetOutputImageExtent(int num, int *extent)
{
  int idx;
  
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "SetOutputImageExtent: Dimensions too large");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num*2; ++idx)
    {
    this->OutputImageExtent[idx] = extent[idx];
    }
}


//----------------------------------------------------------------------------
void vtkImagePadFilter::GetOutputImageExtent(int num, int *extent)
{
  int idx;
  
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "GetOutputImageExtent: Dimensions too large");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num*2; ++idx)
    {
    extent[idx] = this->OutputImageExtent[idx];
    }
}


//----------------------------------------------------------------------------
// Just change the Image extent.
void vtkImagePadFilter::ComputeOutputImageInformation(vtkImageRegion *inRegion,
					      vtkImageRegion *outRegion)
{
  inRegion = inRegion;
  outRegion->SetImageExtent(VTK_IMAGE_DIMENSIONS, this->OutputImageExtent);
}

//----------------------------------------------------------------------------
// Just clip the request.  The subclass may need to overwrite this method.
void vtkImagePadFilter::ComputeRequiredInputRegionExtent(
						 vtkImageRegion *outRegion,
						 vtkImageRegion *inRegion)
{
  int idx;
  int extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int *imageExtent;
  
  outRegion->GetExtent(VTK_IMAGE_DIMENSIONS, extent);
  imageExtent = inRegion->GetImageExtent();

  // Clip
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (extent[idx*2] < imageExtent[idx*2])
      {
      extent[idx*2] = imageExtent[idx*2];
      }
    if (extent[idx*2 + 1] > imageExtent[idx*2 + 1])
      {
      extent[idx*2 + 1] = imageExtent[idx*2 + 1];
      }
    }
  
  inRegion->SetExtent(VTK_IMAGE_DIMENSIONS, extent);
}















