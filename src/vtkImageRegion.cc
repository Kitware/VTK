/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRegion.cc
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
#include "vtkImageRegion.hh"

//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageRegion with no data.
vtkImageRegion::vtkImageRegion()
{
  this->Data = NULL;
  this->DataType = VTK_IMAGE_VOID;
  this->SetAxes4d(VTK_IMAGE_X_AXIS,
		  VTK_IMAGE_Y_AXIS,
		  VTK_IMAGE_Z_AXIS,
		  VTK_IMAGE_TIME_AXIS);
  this->SetBounds4d(0,0, 0,0, 0,0, 0,0);
  this->SetImageBounds4d(0,0, 0,0, 0,0, 0,0);
}


//----------------------------------------------------------------------------
// Description:
// Destructor: Deleting a vtkImageRegion automatically deletes the associated
// vtkImageData.  However, since the data is reference counted, it may not 
// actually be deleted.
vtkImageRegion::~vtkImageRegion()
{
  if (this->Data)
    this->Data->Delete();
}



/*****************************************************************************
  Stuff to treat region as a source
*****************************************************************************/


//----------------------------------------------------------------------------
// Description:
// Right now, the data is used for the new region with no eror checking.
// Don't ask for for a larger region than this one!  This implementation
// also ignores the relative coordinates of the regions.  If this becomes a 
// problem, an execute method that copies the data cound be created.
void vtkImageRegion::UpdateRegion(vtkImageRegion *region)
{
  region->SetDataType(this->GetDataType());
  region->SetData(this->GetData());
}

  
//----------------------------------------------------------------------------
// Description:
// Returns the bounds of the region as the image bounds.
void vtkImageRegion::UpdateImageInformation(vtkImageRegion *region)
{
  region->SetImageBounds4d(this->GetBounds4d());
}



//----------------------------------------------------------------------------
// Description:
// Just the MTime of this region.
unsigned long vtkImageRegion::GetPipelineMTime()
{
  return this->GetMTime();
}




/*****************************************************************************
  Stuff to access region information (4d, 3d, 2d or 1d)
*****************************************************************************/









//----------------------------------------------------------------------------
// Description:
// When dealing with Regions directly (no caches), they can be allocated
// with this method.  It keeps you from having to create a vtkImageData
// object and setting it explicitley.
void vtkImageRegion::Allocate()
{
  this->Modified();

  if (this->Data)
    {
    this->Data->Delete();
    this->Data = NULL;
    }

  this->Data = new vtkImageData;
  this->Data->SetType(this->DataType);
  this->Data->SetBounds(this->AbsoluteBounds);
  this->Data->Allocate();
  
  // Compute the relative increments.
  this->ShuffleAbsoluteToRelative4d(this->Data->GetIncrements(), 
				    this->Increments);
}


//----------------------------------------------------------------------------
// Description:
// You can set the data object explicitly, instead of using the Allocate
// method.  Old data is released, and the region automatically registers
// the new data.  This assumes that the Data has already been allocated,
// and the increments will not change.
void vtkImageRegion::SetData(vtkImageData *data)
{
  if (! data->IsAllocated())
    {
    vtkErrorMacro(<< "SetData:Current implementation requires allocated data");
    return;
    }
  
  this->Modified();
  // data objects have reference counts
  data->Register(this);

  // delete previous data
  if (this->Data)
    {
    this->Data->Delete();
    this->Data = NULL;
    }

  this->Data = data;
  
  // Compute the relative increments.
  this->ShuffleAbsoluteToRelative4d(data->GetIncrements(), 
				    this->Increments);
}



//----------------------------------------------------------------------------
// Description:
// These method return the increments between pixels, rows, images and volumes.
// A Coordinate system relative to "Axes" is used to set the order.
// These values are determined by the actual dimensions of the data stored
// in the vtkImageData object.  "Increments" allows the user to efficiently 
// march through the memory using pointer arithmatic, while keeping the
// actual dimensions of the memory array transparent.  
void vtkImageRegion::GetIncrements4d(int &inc0,int &inc1,int &inc2,int &inc3)
{
  if ( ! this->Data){
    vtkErrorMacro(<<"Data must be set or allocated.");
    return;
  }

  inc0 = this->Increments[0];
  inc1 = this->Increments[1];
  inc2 = this->Increments[2];
  inc3 = this->Increments[3];
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetIncrements3d(int &inc0, int &inc1, int &inc2)
{
  if ( ! this->Data){
    vtkErrorMacro(<<"Data must be set or allocated.");
    return;
  }

  inc0 = this->Increments[0];
  inc1 = this->Increments[1];
  inc2 = this->Increments[2];
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetIncrements2d(int &inc0, int &inc1)
{
  if ( ! this->Data){
    vtkErrorMacro(<<"Data must be set or allocated.");
  }

  inc0 = this->Increments[0];
  inc1 = this->Increments[1];
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetIncrements1d(int &inc0)
{
  if ( ! this->Data){
    vtkErrorMacro(<<"Data must be set or allocated.");
  }

  inc0 = this->Increments[0];
}
//----------------------------------------------------------------------------
int *vtkImageRegion::GetIncrements()
{
  if ( ! this->Data){
    vtkErrorMacro(<<"Data must be set or allocated.");
    return NULL;
  }

  return this->Increments;
}


//----------------------------------------------------------------------------
// Description:
// These methods return pointers at locations in the region.  The coordinates
// of the location are in pixel units and are relative to the absolute
// origin of the whole image. The region just forwards the message
// to its vtkImageData object.
void *vtkImageRegion::GetVoidPointer4d(int coordinates[4])
{
  int absoluteCoordinates[4];
  
  if ( ! this->Data){
    vtkErrorMacro(<<"Data must be set or allocated.");
    return NULL;
  }

  // Convert into data coordinates
  this->ShuffleRelativeToAbsolute4d(coordinates, absoluteCoordinates);
  
  return this->Data->GetVoidPointer(absoluteCoordinates);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer3d(int coordinates[3])
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = coordinates[0];
  coords[1] = coordinates[1];
  coords[2] = coordinates[2];
  coords[3] = this->DefaultCoordinate3;
  
  return this->GetVoidPointer4d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer2d(int coordinates[2])
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = coordinates[0];
  coords[1] = coordinates[1];
  coords[2] = this->DefaultCoordinate2;
  coords[3] = this->DefaultCoordinate3;
  
  return this->GetVoidPointer4d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer1d(int coordinates[1])
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = coordinates[0];
  coords[1] = this->DefaultCoordinate1;
  coords[2] = this->DefaultCoordinate2;
  coords[3] = this->DefaultCoordinate3;
  
  return this->GetVoidPointer4d(coords);
}


//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer4d()
{
  int coords[VTK_IMAGE_DIMENSIONS];

  coords[0] = this->Bounds[0];
  coords[1] = this->Bounds[2];
  coords[2] = this->Bounds[4];
  coords[3] = this->Bounds[6];
  
  return this->GetVoidPointer4d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer3d()
{
  int coords[VTK_IMAGE_DIMENSIONS];

  coords[0] = this->Bounds[0];
  coords[1] = this->Bounds[2];
  coords[2] = this->Bounds[4];
  coords[3] = this->DefaultCoordinate3;
  
  return this->GetVoidPointer4d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer2d()
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = this->Bounds[0];
  coords[1] = this->Bounds[2];
  coords[2] = this->DefaultCoordinate2;
  coords[3] = this->DefaultCoordinate3;
  
  return this->GetVoidPointer4d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer1d()
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = this->Bounds[0];
  coords[1] = this->DefaultCoordinate1;
  coords[2] = this->DefaultCoordinate2;
  coords[3] = this->DefaultCoordinate3;
  
  return this->GetVoidPointer4d(coords);
}








//----------------------------------------------------------------------------
// Description:
// Reorder the axes
void vtkImageRegion::SetAxes4d(int *axes)
{
  int relativeAxis, absoluteAxis, idx;
  int modified = 0;
  
  // Error checking
  for (relativeAxis = 0; relativeAxis < 4; ++relativeAxis)
    {
    absoluteAxis = axes[relativeAxis];
    // make sure we have a defined axis.
    if (absoluteAxis < 0 || absoluteAxis >= 4)
      {
      vtkErrorMacro(<< "SetAxes: " << absoluteAxis << " out of range.");
      return;
      }
    // Check to see if anything has realy changed
    if (absoluteAxis != this->Axes[relativeAxis])
      {
      modified = 1;
      }
    // make sure no axis occurs twice
    for (idx = 0; idx < relativeAxis; ++idx)
      {
      if (absoluteAxis == axes[idx])
	{
	vtkErrorMacro(<< "SetAxes: Axis " << absoluteAxis << " occurs twice.");
	return;
	}
      }
    }

  // Do nothing if nothing has been modified.
  if (! modified)
    {
    return;
    }
  this->Modified();
  
  // Change the bounds
  for (relativeAxis = 0; relativeAxis < 4; ++relativeAxis)
    {
    this->Axes[relativeAxis] = axes[relativeAxis];
    }
  
  // Any DefaultCoordinates set must be invalid now.
  this->ResetDefaultCoordinates(4);
  
  // Recompute relative IVARs
  this->ShuffleBoundsAbsoluteToRelative4d(this->AbsoluteBounds, this->Bounds);
  this->ShuffleBoundsAbsoluteToRelative4d(this->AbsoluteImageBounds, 
					  this->ImageBounds);
  if (this->Data)
    {
    this->ShuffleAbsoluteToRelative4d(this->Data->GetIncrements(), 
				      this->Increments);
    }
}
//----------------------------------------------------------------------------
void vtkImageRegion::SetAxes4d(int axis0, int axis1, int axis2, int axis3)
{
  int axes[4];
  
  axes[0] = axis0;
  axes[1] = axis1;
  axes[2] = axis2;
  axes[3] = axis3;
  this->SetAxes4d(axes);
}








//----------------------------------------------------------------------------
void vtkImageRegion::GetAxes4d(int &axis0, int &axis1, int &axis2, int &axis3)
{
  axis0 = this->Axes[0];
  axis1 = this->Axes[1];
  axis2 = this->Axes[2];
  axis3 = this->Axes[3];
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetAxes3d(int &axis0, int &axis1, int &axis2)
{
  axis0 = this->Axes[0];
  axis1 = this->Axes[1];
  axis2 = this->Axes[2];
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetAxes2d(int &axis0, int &axis1)
{
  axis0 = this->Axes[0];
  axis1 = this->Axes[1];
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetAxes1d(int &axis0)
{
  axis0 = this->Axes[0];
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetAxes(int *axes, int dim)
{
  int idx;
  
  for (idx = 0; idx < dim; ++idx)
    {
    axes[idx] = this->Axes[idx];
    }
}


//----------------------------------------------------------------------------
// A helper function that sets the DefaultCoordinates to there default values.
// Maybe DefaultCoordinates should be stored in an array.
void vtkImageRegion::ResetDefaultCoordinates(int dim)
{
  if (dim > 1) 
    this->DefaultCoordinate1 = this->Bounds[2];
  if (dim > 2) 
    this->DefaultCoordinate2 = this->Bounds[4];
  if (dim > 3)
    this->DefaultCoordinate3 = this->Bounds[6];
}






//----------------------------------------------------------------------------
// Description:
// These methods set the bounds of the region.
void vtkImageRegion::SetBounds(int *bounds, int dim)
{
  int idx;
  int boundsDim = dim * 2;
  
  for (idx = 0; idx < boundsDim; ++idx)
    {
    this->Bounds[idx] = bounds[idx];
    }
  this->ShuffleBoundsRelativeToAbsolute4d(this->Bounds, this->AbsoluteBounds);
  this->ResetDefaultCoordinates(dim);
}
//----------------------------------------------------------------------------
void vtkImageRegion::SetBounds4d(int min0, int max0, int min1, int max1, 
				 int min2, int max2, int min3, int max3)
{
  this->Bounds[0] = min0;   this->Bounds[1] = max0;   
  this->Bounds[2] = min1;   this->Bounds[3] = max1;   
  this->Bounds[4] = min2;   this->Bounds[5] = max2;   
  this->Bounds[6] = min3;   this->Bounds[7] = max3;   
  this->ShuffleBoundsRelativeToAbsolute4d(this->Bounds, this->AbsoluteBounds);
  this->ResetDefaultCoordinates(4);
}
//----------------------------------------------------------------------------
void vtkImageRegion::SetBounds3d(int min0, int max0, int min1, int max1, 
				 int min2, int max2)
{
  this->Bounds[0] = min0;   this->Bounds[1] = max0;   
  this->Bounds[2] = min1;   this->Bounds[3] = max1;   
  this->Bounds[4] = min2;   this->Bounds[5] = max2;   
  this->ShuffleBoundsRelativeToAbsolute4d(this->Bounds, this->AbsoluteBounds);
  this->ResetDefaultCoordinates(3);
}
//----------------------------------------------------------------------------
void vtkImageRegion::SetBounds2d(int min0, int max0, int min1, int max1)
{
  this->Bounds[0] = min0;   this->Bounds[1] = max0;   
  this->Bounds[2] = min1;   this->Bounds[3] = max1;   
  this->ShuffleBoundsRelativeToAbsolute4d(this->Bounds, this->AbsoluteBounds);
  this->ResetDefaultCoordinates(2);
}
//----------------------------------------------------------------------------
void vtkImageRegion::SetBounds1d(int min0, int max0)
{
  this->Bounds[0] = min0;   this->Bounds[1] = max0;   
  this->ShuffleBoundsRelativeToAbsolute4d(this->Bounds, this->AbsoluteBounds);
  this->ResetDefaultCoordinates(1);
}

//----------------------------------------------------------------------------
// Description:
// These methods get the bounds of the region.
void vtkImageRegion::GetBounds(int *bounds, int dim)
{
  int idx;
  
  dim *= 2;
  for (idx = 0; idx < dim; ++idx)
    {
    bounds[idx] = this->Bounds[idx];
    }
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetBounds4d(int &min0, int &max0, int &min1, int &max1, 
				 int &min2, int &max2, int &min3, int &max3)
{
  min0 = this->Bounds[0];   max0 = this->Bounds[1];   
  min1 = this->Bounds[2];   max1 = this->Bounds[3];   
  min2 = this->Bounds[4];   max2 = this->Bounds[5];   
  min3 = this->Bounds[6];   max3 = this->Bounds[7];   
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetBounds3d(int &min0, int &max0, int &min1, int &max1, 
				 int &min2, int &max2)
{
  min0 = this->Bounds[0];   max0 = this->Bounds[1];   
  min1 = this->Bounds[2];   max1 = this->Bounds[3];   
  min2 = this->Bounds[4];   max2 = this->Bounds[5];   
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetBounds2d(int &min0, int &max0, int &min1, int &max1)
{
  min0 = this->Bounds[0];   max0 = this->Bounds[1];   
  min1 = this->Bounds[2];   max1 = this->Bounds[3];   
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetBounds1d(int &min0, int &max0)
{
  min0 = this->Bounds[0];   max0 = this->Bounds[1];   
}











//----------------------------------------------------------------------------
// Description:
// These methods set the ImageBounds of the region.
void vtkImageRegion::SetImageBounds(int *bounds, int dim)
{
  int idx;
  int boundsDim = dim * 2;
  
  for (idx = 0; idx < boundsDim; ++idx)
    {
    this->ImageBounds[idx] = bounds[idx];
    }
  this->ShuffleBoundsRelativeToAbsolute4d(this->ImageBounds, 
					  this->AbsoluteImageBounds);
  this->ResetDefaultCoordinates(dim);
}
//----------------------------------------------------------------------------
void vtkImageRegion::SetImageBounds4d(int min0, int max0, int min1, int max1, 
				      int min2, int max2, int min3, int max3)
{
  this->ImageBounds[0] = min0;   this->ImageBounds[1] = max0;   
  this->ImageBounds[2] = min1;   this->ImageBounds[3] = max1;   
  this->ImageBounds[4] = min2;   this->ImageBounds[5] = max2;   
  this->ImageBounds[6] = min3;   this->ImageBounds[7] = max3;   
  this->ShuffleBoundsRelativeToAbsolute4d(this->ImageBounds, 
					  this->AbsoluteImageBounds);
  this->ResetDefaultCoordinates(4);
}
//----------------------------------------------------------------------------
void vtkImageRegion::SetImageBounds3d(int min0, int max0, int min1, int max1, 
				      int min2, int max2)
{
  this->ImageBounds[0] = min0;   this->ImageBounds[1] = max0;   
  this->ImageBounds[2] = min1;   this->ImageBounds[3] = max1;   
  this->ImageBounds[4] = min2;   this->ImageBounds[5] = max2;   
  this->ShuffleBoundsRelativeToAbsolute4d(this->ImageBounds, 
					  this->AbsoluteImageBounds);
  this->ResetDefaultCoordinates(3);
}
//----------------------------------------------------------------------------
void vtkImageRegion::SetImageBounds2d(int min0, int max0, int min1, int max1)
{
  this->ImageBounds[0] = min0;   this->ImageBounds[1] = max0;   
  this->ImageBounds[2] = min1;   this->ImageBounds[3] = max1;   
  this->ShuffleBoundsRelativeToAbsolute4d(this->ImageBounds, 
					  this->AbsoluteImageBounds);
  this->ResetDefaultCoordinates(2);
}
//----------------------------------------------------------------------------
void vtkImageRegion::SetImageBounds1d(int min0, int max0)
{
  this->ImageBounds[0] = min0;   this->ImageBounds[1] = max0;   
  this->ShuffleBoundsRelativeToAbsolute4d(this->ImageBounds, 
					  this->AbsoluteImageBounds);
  this->ResetDefaultCoordinates(1);
}

//----------------------------------------------------------------------------
// Description:
// These methods get the ImageBounds of the region.
void vtkImageRegion::GetImageBounds(int *boundary, int dim)
{
  int idx;
  
  dim *= 2;
  for (idx = 0; idx < dim; ++idx)
    {
    boundary[idx] = this->ImageBounds[idx];
    }
}
//---------------------------------------------------------------------------
void vtkImageRegion::GetImageBounds4d(int &min0,int &max0, int &min1,int &max1,
				      int &min2,int &max2, int &min3,int &max3)
{
  min0 = this->ImageBounds[0];   max0 = this->ImageBounds[1];   
  min1 = this->ImageBounds[2];   max1 = this->ImageBounds[3];   
  min2 = this->ImageBounds[4];   max2 = this->ImageBounds[5];   
  min3 = this->ImageBounds[6];   max3 = this->ImageBounds[7];   
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetImageBounds3d(int &min0,int &max0, int &min1,int &max1,
				      int &min2,int &max2)
{
  min0 = this->ImageBounds[0];   max0 = this->ImageBounds[1];   
  min1 = this->ImageBounds[2];   max1 = this->ImageBounds[3];   
  min2 = this->ImageBounds[4];   max2 = this->ImageBounds[5];   
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetImageBounds2d(int &min0,int &max0, int &min1,int &max1)
{
  min0 = this->ImageBounds[0];   max0 = this->ImageBounds[1];   
  min1 = this->ImageBounds[2];   max1 = this->ImageBounds[3];   
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetImageBounds1d(int &min0,int &max0)
{
  min0 = this->ImageBounds[0];   max0 = this->ImageBounds[1];   
}











//----------------------------------------------------------------------------
// Description:
// Convert 4d vector (not bounds!) from absolute coordinates into
// relative coordinate system of region.
void vtkImageRegion::ShuffleRelativeToAbsolute4d(int *relative, int *absolute)
{
  int idx;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    absolute[this->Axes[idx]] = relative[idx];
    }
}

//----------------------------------------------------------------------------
// Description:
// Convert 4d vector (not bounds!) from relative coordinates into
// absolute coordinate system.
void vtkImageRegion::ShuffleAbsoluteToRelative4d(int *absolute, int *relative)
{
  int idx;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    relative[idx] = absolute[this->Axes[idx]];
    }
}



//----------------------------------------------------------------------------
// Description:
// Convert 4d bounds from absolute coordinates into
// relative coordinate system of region.
void vtkImageRegion::ShuffleBoundsRelativeToAbsolute4d(int *relative, 
						       int *absolute)
{
  int idx;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    absolute[this->Axes[idx]*2] = relative[idx*2];
    absolute[this->Axes[idx]*2+1] = relative[idx*2+1];
    }
}

//----------------------------------------------------------------------------
// Description:
// Convert 4d bounds from relative coordinates into
// absolute coordinate system.
void vtkImageRegion::ShuffleBoundsAbsoluteToRelative4d(int *absolute, 
						       int *relative)
{
  int idx;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    relative[idx*2] = absolute[this->Axes[idx]*2];
    relative[idx*2+1] = absolute[this->Axes[idx]*2+1];
    }
}



//----------------------------------------------------------------------------
void vtkImageRegion::ImportMemory(void *ptr)
{
  ptr = ptr;
  
  vtkErrorMacro(<< "Importindg memory is not implemented yet.");
}


//----------------------------------------------------------------------------
void *vtkImageRegion::ExportMemory()
{
  return (void *)(this->Data->GetVoidPointer());
}





