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
#include "vtkImageCache.h"
#include "vtkImageRegion.h"
#include "vtkImagePadFilter.h"



//----------------------------------------------------------------------------
// Description:
// Constructor sets default values
vtkImagePadFilter::vtkImagePadFilter()
{
  int idx;

  this->NumberOfExecutionAxes = 5;
  
  // Initialize output image extent to one pixel (origin)
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->OutputWholeExtent[idx * 2] = 0;
    this->OutputWholeExtent[idx * 2 + 1] = 0;
    }
  this->OutputNumberOfScalarComponents = 1;
}

//----------------------------------------------------------------------------
void vtkImagePadFilter::SetOutputWholeExtent(int extent[8])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 8; ++idx)
    {
    if (this->OutputWholeExtent[idx] != extent[idx])
      {
      this->OutputWholeExtent[idx] = extent[idx];
      this->Modified();
      }
    }
  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImagePadFilter::SetOutputWholeExtent(int minX, int maxX, 
					     int minY, int maxY,
					     int minZ, int maxZ, 
					     int minT, int maxT)
{
  int extent[8];
  
  extent[0] = minX;  extent[1] = maxX;
  extent[2] = minY;  extent[3] = maxY;
  extent[4] = minZ;  extent[5] = maxZ;
  extent[6] = minT;  extent[7] = maxT;
  this->SetOutputWholeExtent(extent);
}


//----------------------------------------------------------------------------
void vtkImagePadFilter::GetOutputWholeExtent(int extent[8])
{
  int idx;
  
  for (idx = 0; idx < 8; ++idx)
    {
    extent[idx] = this->OutputWholeExtent[idx];
    }
}


//----------------------------------------------------------------------------
// Just change the Image extent.
void vtkImagePadFilter::ExecuteImageInformation(vtkImageCache *in,
						vtkImageCache *out)
{
  in = in;
  out->SetWholeExtent(this->OutputWholeExtent);
  out->SetNumberOfScalarComponents(this->OutputNumberOfScalarComponents);
}

//----------------------------------------------------------------------------
// Just clip the request.  The subclass may need to overwrite this method.
void vtkImagePadFilter::ComputeRequiredInputUpdateExtent(vtkImageCache *out,
							 vtkImageCache *in)
{
  int idx;
  int extent[8];
  int *wholeExtent;
  
  // handle XYZT
  out->GetUpdateExtent(extent);
  wholeExtent = in->GetWholeExtent();
  // Clip
  for (idx = 0; idx < 4; ++idx)
    {
    if (extent[idx*2] < wholeExtent[idx*2])
      {
      extent[idx*2] = wholeExtent[idx*2];
      }
    if (extent[idx*2 + 1] > wholeExtent[idx*2 + 1])
      {
      extent[idx*2 + 1] = wholeExtent[idx*2 + 1];
      }
    }
  in->SetUpdateExtent(extent);
  
  // Components are handled automatically (see ExecuteImageInformation)
}















