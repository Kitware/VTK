/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage4dMatFilter.cxx
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
#include "vtkImage4dMatFilter.h"


//----------------------------------------------------------------------------
vtkImage4dMatFilter::vtkImage4dMatFilter()
{
  this->Input = NULL;
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, 
		VTK_IMAGE_Z_AXIS, VTK_IMAGE_TIME_AXIS);
  this->SetBorderWidths(1, 1, 0, 0);
  this->SetBorderValue(0.0);
}





//----------------------------------------------------------------------------
void vtkImage4dMatFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);
  if (this->Input)
    {
    os << indent << "Input: (" << this->Input << ")\n";
    }
  else
    {
    os << indent << "Input: NULL\n";
    }
  
  os << indent << "Axes: (" 
     << vtkImageAxisNameMacro(this->Axes[0]) << ", "
     << vtkImageAxisNameMacro(this->Axes[1]) << ", "
     << vtkImageAxisNameMacro(this->Axes[2]) << ", "
     << vtkImageAxisNameMacro(this->Axes[3]) << ")/n";
  os << indent << "BorderWidths: (" << this->BorderWidths[0] << ", "
     << this->BorderWidths[1] << ", "<< this->BorderWidths[2] << ", "
     << this->BorderWidths[3] << ")/n";
  os << indent << "BorderValue: " << this->BorderValue << "\n";
  
}
  


//----------------------------------------------------------------------------
// Template function that adds one bounding box region of mat.
// There is really no need for two templated functions.
template <class T>
void vtkImage4dMatFilterExecute2(vtkImage4dMatFilter *self, 
				 vtkImageRegion *region, T *ptr,
				 int *bounds)
{
  int idx0, idx1, idx2, idx3;
  int min0, max0, min1, max1, min2, max2, min3, max3;
  T *ptr0, *ptr1, *ptr2, *ptr3;
  int inc0, inc1, inc2, inc3;
  T value;
  
  value = (T)(self->GetBorderValue());
  
  min0 = bounds[0];
  max0 = bounds[1];
  min1 = bounds[2];
  max1 = bounds[3];
  min2 = bounds[4];
  max2 = bounds[5];
  min3 = bounds[6];
  max3 = bounds[7];
  region->GetIncrements4d(inc0, inc1, inc2, inc3);
  ptr = (T *)(region->GetVoidPointer4d(min0, min1, min2, min3));
  
  ptr3 = ptr;
  for (idx3 = min3; idx3 <= max3; ++idx3)
    {
    ptr2 = ptr3;
    for (idx2 = min2; idx2 <= max2; ++idx2)
      {
      ptr1 = ptr2;
      for (idx1 = min1; idx1 <= max1; ++idx1)
	{
	ptr0 = ptr1;
	for (idx0 = min0; idx0 <= max0; ++idx0)
	  {
	  *ptr0 = value;
	  
	  ptr0 += inc0;
	  }
	ptr1 += inc1;
	}
      ptr2 += inc2;
      }
    ptr3 += inc3;
    }
}


//----------------------------------------------------------------------------
// Template function that adds a mat to any data type.
template <class T>
void vtkImage4dMatFilterExecute(vtkImage4dMatFilter *self, 
				vtkImageRegion *region, T *ptr)
{
  int mat[8];
  int center[8];
  int bounds[8];
  int *borderWidths;
  int *imageBounds;
  int idxAxes, idx;
  
  imageBounds = region->GetImageBounds();
  region->GetBounds(bounds);
  borderWidths = self->GetBorderWidths();
  
  // compute the unmatted bounds
  for (idxAxes = 0; idxAxes < 4; ++idxAxes)
    {
    imageBounds[idxAxes*2] += borderWidths[idxAxes];
    imageBounds[idxAxes*2+1] -= borderWidths[idxAxes];
    if (imageBounds[idxAxes*2] > imageBounds[idxAxes*2+1])
      {
      // Special case border covers whole region.
      vtkImage4dMatFilterExecute2(self, region, ptr, bounds);
      return;
      }
    center[idxAxes*2] = (bounds[idxAxes*2] > imageBounds[idxAxes*2]) ?
      bounds[idxAxes*2] : imageBounds[idxAxes*2];
    center[idxAxes*2+1] = (bounds[idxAxes*2+1] < imageBounds[idxAxes*2+1]) ?
      bounds[idxAxes*2+1] : imageBounds[idxAxes*2+1];
    }
      
  for (idxAxes = 0; idxAxes < 4; ++idxAxes)
    {
    // Check the lower part for a mat region.
    if (center[idxAxes*2] > bounds[idxAxes*2])
      {
      // Compute mat
      for (idx = 0; idx < 8; ++idx)
	{
	mat[idx] = bounds[idx];
	}
      mat[idxAxes*2+1] = center[idxAxes*2] - 1;
      // Fill with border value
      vtkImage4dMatFilterExecute2(self, region, ptr, mat);
      // shrink bounds progressively to center to avoid overlap
      bounds[idxAxes*2] = center[idxAxes*2];
      }
    // Check the upper part for a mat region.
    if (center[idxAxes*2+1] < bounds[idxAxes*2+1])
      {
      // Compute mat
      for (idx = 0; idx < 8; ++idx)
	{
	mat[idx] = bounds[idx];
	}
      mat[idxAxes*2] = center[idxAxes*2+1] + 1;
      // Fill with border value
      vtkImage4dMatFilterExecute2(self, region, ptr, mat);
      // progressively shink bounds to center  to avoid overlap
      bounds[idxAxes*2+1] = center[idxAxes*2+1];
      }
    }
}

    
    

//----------------------------------------------------------------------------
// Description:
void vtkImage4dMatFilter::UpdateRegion(vtkImageRegion *region)
{
  int axesSave[VTK_IMAGE_DIMENSIONS];
  int *imageBounds, *bounds;
  int idx, flag;
  void *ptr;
    
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "UpdateRegion: No Input");
    return;
    }

  // Change to local coordinate system.
  region->GetAxes(axesSave);
  region->SetAxes4d(this->Axes);

  // Get the region from input
  this->Input->UpdateRegion(region);
  
  // Check to see if this region needs to be matted.
  bounds = region->GetBounds();
  imageBounds = region->GetImageBounds();
  flag = 0;
  for(idx = 0; idx < 4; ++idx)
    {
    if ((bounds[idx*2] < imageBounds[idx*2] + this->BorderWidths[idx]) ||
	(bounds[idx*2+1] > imageBounds[idx*2+1] - this->BorderWidths[idx]))
      {
      flag = 1;
      break;
      }
    }
  
  if ( ! flag)
    {
    // just pass the region along.
    return;
    }
  
  // Get rid of all references to the data.
  region->MakeWritable();
  
  // Add the border
  ptr = region->GetVoidPointer();
  switch (region->GetDataType())
    {
    case VTK_IMAGE_FLOAT:
      vtkImage4dMatFilterExecute(this, region, (float *)(ptr));
      break;
    case VTK_IMAGE_INT:
      vtkImage4dMatFilterExecute(this, region, (int *)(ptr));
      break;
    case VTK_IMAGE_SHORT:
      vtkImage4dMatFilterExecute(this, region, (short *)(ptr));
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImage4dMatFilterExecute(this, region, (unsigned short *)(ptr));
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImage4dMatFilterExecute(this, region, (unsigned char *)(ptr));
      break;
    default:
      vtkErrorMacro(<< "UpdateRegion: Cannot handle DataType.\n");
    }         
  
}


//----------------------------------------------------------------------------
// Description:
// Image information is same as input
void vtkImage4dMatFilter::UpdateImageInformation(vtkImageRegion *region)
{
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "UpdateImageInformation: No Input");
    return;
    }
  
  this->Input->UpdateImageInformation(region);
}
  



//----------------------------------------------------------------------------
// Description:
// Returns PipelineMTime of input.
unsigned long vtkImage4dMatFilter::GetPipelineMTime()
{
  if ( ! this->Input)
    {
    vtkWarningMacro(<< "GetPipelineMTime: No Input");
    return this->GetMTime();
    }

  return this->Input->GetPipelineMTime();
}


  
//----------------------------------------------------------------------------
// Description:
// Returns DataType of input.
int vtkImage4dMatFilter::GetDataType()
{
  if ( ! this->Input)
    {
    vtkWarningMacro(<< "GetDataType: No Input");
    return VTK_IMAGE_VOID;
    }

  return this->Input->GetDataType();
}



//----------------------------------------------------------------------------
// Description:
// Other widths are set to 0
void vtkImage4dMatFilter::SetBorderWidths1d(int w0)
{
  this->SetBorderWidths(w0, 0, 0, 0);
}

//----------------------------------------------------------------------------
// Description:
// Other widths are set to 0
void vtkImage4dMatFilter::SetBorderWidths2d(int w0, int w1)
{
  this->SetBorderWidths(w0, w1, 0, 0);
}

//----------------------------------------------------------------------------
// Description:
// Other widths are set to 0
void vtkImage4dMatFilter::SetBorderWidths3d(int w0, int w1, int w2)
{
  this->SetBorderWidths(w0, w1, w2, 0);
}







  







