/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMatte.cxx
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
#include "vtkImageRegion.h"
#include "vtkImageMatte.h"


//----------------------------------------------------------------------------
vtkImageMatte::vtkImageMatte()
{
  this->Input = NULL;
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
  // Templated fill function only handles 4 dimensions.
  // We should really have a separate SetAxes to overide Dimensionality.
  // Or have execute recursion termination depend on something else.
  this->SetBorderWidths(1, 1);
  this->SetBorderValue(0.0);
  this->NumberOfExecutionAxes = VTK_IMAGE_DIMENSIONS;
}





//----------------------------------------------------------------------------
void vtkImageMatte::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkImageInPlaceFilter::PrintSelf(os,indent);
  os << indent << "BorderWidths: (" << this->BorderWidths[0];
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << this->BorderWidths[idx];
    }
  os << "\n";

  os << indent << "BorderValue: " << this->BorderValue << "\n";
}
  


//----------------------------------------------------------------------------
// Split up into finished and border regions.  Fill the border regions.
void vtkImageMatte::Execute(vtkImageRegion *inRegion, 
			    vtkImageRegion *outRegion)
{
  int saveExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int finishedExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int newExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int *imageExtent;
  int idx, idx2;
  
  // Since we do not use inRegion
  inRegion = inRegion;

  // compute the valid middle
  outRegion->GetExtent(VTK_IMAGE_DIMENSIONS, saveExtent);
  imageExtent = outRegion->GetImageExtent();
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    // Start with image extent, shrink the borders.
    finishedExtent[2*idx] = imageExtent[2*idx] + this->BorderWidths[idx];
    finishedExtent[2*idx+1] = imageExtent[2*idx+1] - this->BorderWidths[idx];
    // clip it with the region extent
    if (finishedExtent[2*idx] < saveExtent[2*idx])
      {
      finishedExtent[2*idx] = saveExtent[2*idx];
      }
    if (finishedExtent[2*idx+1] > saveExtent[2*idx+1])
      {
      finishedExtent[2*idx+1] = saveExtent[2*idx+1];
      }
    // If thick borders made the valid region inside out ...
    if (finishedExtent[2*idx] > finishedExtent[2*idx+1])
      {
      // The whole region is replaced
      this->FillRegion(outRegion);
      return;
      }
    }
  
  // Fill border region by region
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    // Copy finished extent
    for (idx2 = 0; idx2 < VTK_IMAGE_EXTENT_DIMENSIONS; ++idx2)
      {
      newExtent[idx2] = finishedExtent[idx2];
      }
    // Set up lower extent
    newExtent[idx*2] = saveExtent[idx*2];
    newExtent[idx*2+1] = finishedExtent[idx*2] - 1;
    if (newExtent[idx*2] <= newExtent[idx*2+1])
      {
      // Fill this extent
      outRegion->SetExtent(VTK_IMAGE_DIMENSIONS, newExtent); 
      this->FillRegion(outRegion);
      // Expand finished
      finishedExtent[idx*2] = newExtent[idx*2];
      }
    // Set up upper extent
    newExtent[idx*2] = finishedExtent[idx*2+1] + 1;
    newExtent[idx*2+1] = saveExtent[idx*2+1];
    if (newExtent[idx*2] <= newExtent[idx*2+1])
      {
      // Fill this extent
      outRegion->SetExtent(VTK_IMAGE_DIMENSIONS, newExtent); 
      this->FillRegion(outRegion);
      // Expand finished
      finishedExtent[idx*2+1] = newExtent[idx*2+1];
      }
    }
}




//----------------------------------------------------------------------------
// Template function that adds one bounding box region of mat.
template <class T>
static void vtkImageMatteFill(vtkImageMatte *self, vtkImageRegion *region, T *ptr)
{
  int idx0, idx1, idx2, idx3;
  int min0, max0, min1, max1, min2, max2, min3, max3;
  T *ptr0, *ptr1, *ptr2, *ptr3;
  int inc0, inc1, inc2, inc3;
  T value;
  
  value = (T)(self->GetBorderValue());
  
  region->GetExtent(min0, max0, min1, max1, min2, max2, min3, max3);
  region->GetIncrements(inc0, inc1, inc2, inc3);
  ptr = (T *)(region->GetScalarPointer(min0, min1, min2, min3));
  
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
void vtkImageMatte::FillRegion(vtkImageRegion *region)
{
  void *ptr;
  
  ptr = region->GetScalarPointer();
  switch (region->GetScalarType()) {
    case VTK_FLOAT:
      vtkImageMatteFill(this, region, (float *)(ptr));
      break;
    case VTK_INT:
      vtkImageMatteFill(this, region, (int *)(ptr));
      break;
    case VTK_SHORT:
      vtkImageMatteFill(this, region, (short *)(ptr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageMatteFill(this, region, (unsigned short *)(ptr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageMatteFill(this, region, (unsigned char *)(ptr));
      break;
    default:
      vtkErrorMacro(<< "FillRegion: Cannot handle ScalarType.\n");
    }         
  
}



//----------------------------------------------------------------------------
void vtkImageMatte::SetBorderWidths(int num, int *widths)
{
  int idx;

  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (idx < num)
      {
      this->BorderWidths[idx] = widths[idx];      
      }
    else
      {
      this->BorderWidths[idx] = 0;
      }
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageMatte::GetBorderWidths(int num, int *widths)
{
  int idx;
  
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "GetBorderWidths: Requesting too many dimensions.");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    widths[idx] = this->BorderWidths[idx];
    }
}


