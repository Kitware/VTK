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
  this->SetAxes5d(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS,
		VTK_IMAGE_Z_AXIS, VTK_IMAGE_TIME_AXIS,
		VTK_IMAGE_COMPONENT_AXIS);
  this->SetBounds5d(0,0, 0,0, 0,0, 0,0, 0,0);
  this->SetImageBounds5d(0,0, 0,0, 0,0, 0,0, 0,0);
  this->ResetDefaultCoordinates(5);
  this->SetAspectRatio5d(0.0, 0.0, 0.0, 0.0, 0.0);
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


//----------------------------------------------------------------------------
// Description:
// Destructor: Deleting a vtkImageRegion automatically deletes the associated
// vtkImageData.  However, since the data is reference counted, it may not 
// actually be deleted.
void vtkImageRegion::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);
  os << indent << "Axes: (";
  os << vtkImageRegionAxisNameMacro(this->Axes[0]) << ", ";
  os << vtkImageRegionAxisNameMacro(this->Axes[1]) << ", ";
  os << vtkImageRegionAxisNameMacro(this->Axes[2]) << ", ";
  os << vtkImageRegionAxisNameMacro(this->Axes[3]) << ")\n";
  
  os << indent << "Bounds: (";
  os << this->Bounds[0] << ", " << this->Bounds[1] << ", ";
  os << this->Bounds[2] << ", " << this->Bounds[3] << ", ";
  os << this->Bounds[4] << ", " << this->Bounds[5] << ", ";
  os << this->Bounds[6] << ", " << this->Bounds[7] << ")\n";
  
  os << indent << "Default Coordinates: (";
  os << this->DefaultCoordinate0 << ", ";
  os << this->DefaultCoordinate1 << ", ";
  os << this->DefaultCoordinate2 << ", ";
  os << this->DefaultCoordinate3 << ")\n";
  
  os << indent << "ImageBounds: (";
  os << this->ImageBounds[0] << ", " << this->ImageBounds[1] << ", ";
  os << this->ImageBounds[2] << ", " << this->ImageBounds[3] << ", ";
  os << this->ImageBounds[4] << ", " << this->ImageBounds[5] << ", ";
  os << this->ImageBounds[6] << ", " << this->ImageBounds[7] << ")\n";

  os << indent << "AspectRatio: (";
  os << this->AspectRatio[0] << ", ";
  os << this->AspectRatio[1] << ", ";
  os << this->AspectRatio[2] << ", ";
  os << this->AspectRatio[3] << ")\n";

  if ( ! this->Data)
    {
    os << indent << "Data: NULL\n";
    }
  else
    {
    os << indent << "Data:\n";
    this->Data->PrintSelf(os,indent.GetNextIndent());
    }
}
  

/*****************************************************************************
  Stuff for copying regions (double templated).
*****************************************************************************/

//----------------------------------------------------------------------------
// Description:
// Returns the number of references that exist to this regions data.
// If reference count is 1 then you can modify the data without worrying.
int vtkImageRegion::GetReferenceCount()
{
  return this->Data->GetReferenceCount();
}

  
//----------------------------------------------------------------------------
// Second templated function for copying.
template <class IT, class OT>
void vtkImageRegionCopyRegionData(vtkImageRegion *outRegion, OT *outPtr,
				  vtkImageRegion *inRegion, IT *inPtr)
{
  IT *inPtr0, *inPtr1, *inPtr2, *inPtr3, *inPtr4;
  OT *outPtr0, *outPtr1, *outPtr2, *outPtr3, *outPtr4;
  int inInc0, inInc1, inInc2, inInc3, inInc4;
  int outInc0, outInc1, outInc2, outInc3, outInc4;
  int outMin0, outMax0, outMin1, outMax1, 
    outMin2, outMax2, outMin3, outMax3, outMin4, outMax4;
  int idx0, idx1, idx2, idx3, idx4;

  // Get information to loop through data.
  inRegion->GetIncrements5d(inInc0, inInc1, inInc2, inInc3, inInc4);
  outRegion->GetIncrements5d(outInc0, outInc1, outInc2, outInc3, outInc4);
  outRegion->GetBounds5d(outMin0, outMax0, outMin1, outMax1, 
			 outMin2, outMax2, outMin3, outMax3, outMin4, outMax4);
  
  inPtr4 = inPtr;
  outPtr4 = outPtr;
  for (idx4 = outMin4; idx4 <= outMax4; ++idx4)
    {
    inPtr3 = inPtr4;
    outPtr3 = outPtr4;
    for (idx3 = outMin3; idx3 <= outMax3; ++idx3)
      {
      inPtr2 = inPtr3;
      outPtr2 = outPtr3;
      for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
	{
	inPtr1 = inPtr2;
	outPtr1 = outPtr2;
	for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
	  {
	  inPtr0 = inPtr1;
	  outPtr0 = outPtr1;
	  for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
	    {
	    *outPtr0 = (OT)(*inPtr0);
	    inPtr0 += inInc0;
	    outPtr0 += outInc0;
	    }
	  inPtr1 += inInc1;
	  outPtr1 += outInc1;
	  }
	inPtr2 += inInc2;
	outPtr2 += outInc2;
	}
      inPtr3 += inInc3;
      outPtr3 += outInc3;
      }
    inPtr4 += inInc4;
    outPtr4 += outInc4;
    }
}
  
  

//----------------------------------------------------------------------------
// First templated function for copying.
template <class T>
void vtkImageRegionCopyRegionData(vtkImageRegion *self,
				  vtkImageRegion *inRegion, T *inPtr)
{
  void *outPtr;
  
  outPtr = self->GetVoidPointer();
  
  switch (self->GetDataType())
    {
    case VTK_IMAGE_FLOAT:
      vtkImageRegionCopyRegionData(self, (float *)(outPtr), inRegion, inPtr);
      break;
    case VTK_IMAGE_INT:
      vtkImageRegionCopyRegionData(self, (int *)(outPtr), inRegion, inPtr);
      break;
    case VTK_IMAGE_SHORT:
      vtkImageRegionCopyRegionData(self, (short *)(outPtr), inRegion, inPtr);
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImageRegionCopyRegionData(self, (unsigned short *)(outPtr), 
				   inRegion, inPtr);
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImageRegionCopyRegionData(self, (unsigned char *)(outPtr), 
				   inRegion, inPtr);
      break;
    default:
      cerr << "vtkImageRegionCopyRegionData: Cannot handle DataType.\n\n";
    }   
}

//----------------------------------------------------------------------------
// Description:
// Copies data from a region into this region (converting data type).
// It is a simple cast, and will not deal with float to unsigned char
// inteligently.
void vtkImageRegion::CopyRegionData(vtkImageRegion *region)
{
  void *inPtr;
  int *inBounds, *outBounds;
  int origin[VTK_IMAGE_DIMENSIONS];
  int idx;
  
  // Make sure our bounds are contained in the new region.
  inBounds = region->GetBounds();
  outBounds = this->GetBounds();
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (outBounds[2*idx] < inBounds[2*idx] ||
	outBounds[2*idx+1] > inBounds[2*idx+1])
      {
      vtkErrorMacro(<< "CopyRegionData: Bounds mismatch.");
      return;
      }
    origin[idx] = outBounds[2*idx];
    }

  // If the data type is not set, default to same as input.
  if (this->GetDataType() == VTK_IMAGE_VOID)
    {
    this->SetDataType(region->GetDataType());
    }
  
  // Make sure the region is allocated
  if ( ! this->IsAllocated())
    {
    this->Allocate();
    }
  if ( ! this->IsAllocated())
    {
    vtkErrorMacro(<< "Could not allocate region.");
    return;
    }
  
  inPtr = region->GetVoidPointer(origin);
  
  switch (region->GetDataType())
    {
    case VTK_IMAGE_FLOAT:
      vtkImageRegionCopyRegionData(this, region, (float *)(inPtr));
      break;
    case VTK_IMAGE_INT:
      vtkImageRegionCopyRegionData(this, region, (int *)(inPtr));
      break;
    case VTK_IMAGE_SHORT:
      vtkImageRegionCopyRegionData(this, region, (short *)(inPtr));
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImageRegionCopyRegionData(this, region, (unsigned short *)(inPtr));
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImageRegionCopyRegionData(this, region, (unsigned char *)(inPtr));
      break;
    default:
      vtkErrorMacro(<< "CopyRegionData: Cannot handle DataType.");
    }   
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
  this->UpdateImageInformation(region);
  region->SetDataType(this->GetDataType());
  region->SetData(this->GetData());
}

  
//----------------------------------------------------------------------------
// Description:
// Returns the bounds of the region as the image bounds.
void vtkImageRegion::UpdateImageInformation(vtkImageRegion *region)
{
  int axesSave[VTK_IMAGE_DIMENSIONS];
  
  // Save coordinate system
  region->GetAxes(axesSave);
  // convert to this regions coordinate system
  region->SetAxes(this->GetAxes());
  // Set the bounds
  region->SetImageBounds(this->GetBounds());
  // Restore coordinate system to the way it was.
  region->SetAxes(axesSave);
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
// Convert 4d vector (not bounds!) from absolute coordinates into
// relative coordinate system of region.
template <class T>
void vtkImageRegionShuffleRelativeToAbsolute(T *relative, T *absolute, int *axes)
{
  int idx;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    absolute[axes[idx]] = relative[idx];
    }
}

//----------------------------------------------------------------------------
// Description:
// Convert 4d vector (not bounds!) from relative coordinates into
// absolute coordinate system.
template <class T>
void vtkImageRegionShuffleAbsoluteToRelative(T *absolute, T *relative, int *axes)
{
  int idx;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    relative[idx] = absolute[axes[idx]];
    }
}



//----------------------------------------------------------------------------
// Description:
// Convert 4d bounds from absolute coordinates into
// relative coordinate system of region.
void vtkImageRegion::ShuffleBoundsRelativeToAbsolute(int *relative, 
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
void vtkImageRegion::ShuffleBoundsAbsoluteToRelative(int *absolute, 
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
  vtkImageRegionShuffleAbsoluteToRelative(this->Data->GetIncrements(), 
					  this->Increments, this->Axes);
}


//----------------------------------------------------------------------------
// Description:
// Release any data in the region
void vtkImageRegion::ReleaseData()
{
  this->Modified();

  if (this->Data)
    {
    this->Data->Delete();
    this->Data = NULL;
    }

  this->DataType = VTK_IMAGE_VOID;
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
  vtkImageRegionShuffleAbsoluteToRelative(data->GetIncrements(), 
					  this->Increments, this->Axes);
}



//----------------------------------------------------------------------------
// Description:
// These method return the increments between pixels, rows, images and volumes.
// A Coordinate system relative to "Axes" is used to set the order.
// These values are determined by the actual dimensions of the data stored
// in the vtkImageData object.  "Increments" allows the user to efficiently 
// march through the memory using pointer arithmatic, while keeping the
// actual dimensions of the memory array transparent.  
void vtkImageRegion::GetIncrements(int *increments, int dim)
{
  int idx;
  
  if ( ! this->Data){
    vtkErrorMacro(<<"Data must be set or allocated.");
    return;
  }

  for (idx = 0; idx < dim; ++idx)
    {
    increments[idx] = this->Increments[idx];
    }
}


//----------------------------------------------------------------------------
// Description:
// These methods return pointers at locations in the region.  The coordinates
// of the location are in pixel units and are relative to the absolute
// origin of the whole image. The region just forwards the message
// to its vtkImageData object.
void *vtkImageRegion::GetVoidPointer5d(int coordinates[5])
{
  int absoluteCoordinates[VTK_IMAGE_DIMENSIONS];
  
  if ( ! this->Data){
    vtkErrorMacro(<<"Data must be set or allocated.");
    return NULL;
  }

  // Convert into data coordinates
  vtkImageRegionShuffleRelativeToAbsolute(coordinates, absoluteCoordinates, this->Axes);
  
  return this->Data->GetVoidPointer(absoluteCoordinates);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer4d(int coordinates[4])
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = coordinates[0];
  coords[1] = coordinates[1];
  coords[2] = coordinates[2];
  coords[3] = coordinates[3];
  coords[4] = this->DefaultCoordinate4;
  
  return this->GetVoidPointer5d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer3d(int coordinates[3])
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = coordinates[0];
  coords[1] = coordinates[1];
  coords[2] = coordinates[2];
  coords[3] = this->DefaultCoordinate3;
  coords[4] = this->DefaultCoordinate4;
  
  return this->GetVoidPointer5d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer2d(int coordinates[2])
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = coordinates[0];
  coords[1] = coordinates[1];
  coords[2] = this->DefaultCoordinate2;
  coords[3] = this->DefaultCoordinate3;
  coords[4] = this->DefaultCoordinate4;
  
  return this->GetVoidPointer5d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer1d(int coordinates[1])
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = coordinates[0];
  coords[1] = this->DefaultCoordinate1;
  coords[2] = this->DefaultCoordinate2;
  coords[3] = this->DefaultCoordinate3;
  coords[4] = this->DefaultCoordinate4;
  
  return this->GetVoidPointer5d(coords);
}

//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer5d(int c0, int c1, int c2, int c3, int c4)
{
  int coords[5];
  coords[0] = c0;
  coords[1] = c1;
  coords[2] = c2;
  coords[3] = c3;
  coords[4] = c4;
  return this->GetVoidPointer5d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer4d(int c0, int c1, int c2, int c3)
{
  int coords[4];
  coords[0] = c0;
  coords[1] = c1;
  coords[2] = c2;
  coords[3] = c3;
  return this->GetVoidPointer4d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer3d(int c0, int c1, int c2)
{
  int coords[3];
  coords[0] = c0;
  coords[1] = c1;
  coords[2] = c2;
  return this->GetVoidPointer3d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer2d(int c0, int c1)
{
  int coords[2];
  coords[0] = c0;
  coords[1] = c1;
  return this->GetVoidPointer2d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer1d(int c0)
{
  int coords[1];
  coords[0] = c0;
  return this->GetVoidPointer1d(coords);
}

//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer5d()
{
  int coords[VTK_IMAGE_DIMENSIONS];

  coords[0] = this->Bounds[0];
  coords[1] = this->Bounds[2];
  coords[2] = this->Bounds[4];
  coords[3] = this->Bounds[6];
  coords[4] = this->Bounds[8];
  
  return this->GetVoidPointer5d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer4d()
{
  int coords[VTK_IMAGE_DIMENSIONS];

  coords[0] = this->Bounds[0];
  coords[1] = this->Bounds[2];
  coords[2] = this->Bounds[4];
  coords[3] = this->Bounds[6];
  coords[4] = this->DefaultCoordinate4;
  
  return this->GetVoidPointer5d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer3d()
{
  int coords[VTK_IMAGE_DIMENSIONS];

  coords[0] = this->Bounds[0];
  coords[1] = this->Bounds[2];
  coords[2] = this->Bounds[4];
  coords[3] = this->DefaultCoordinate3;
  coords[4] = this->DefaultCoordinate4;
  
  return this->GetVoidPointer5d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer2d()
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = this->Bounds[0];
  coords[1] = this->Bounds[2];
  coords[2] = this->DefaultCoordinate2;
  coords[3] = this->DefaultCoordinate3;
  coords[4] = this->DefaultCoordinate4;
  
  return this->GetVoidPointer5d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer1d()
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = this->Bounds[0];
  coords[1] = this->DefaultCoordinate1;
  coords[2] = this->DefaultCoordinate2;
  coords[3] = this->DefaultCoordinate3;
  coords[4] = this->DefaultCoordinate4;
  
  return this->GetVoidPointer5d(coords);
}





//----------------------------------------------------------------------------
void vtkImageRegion::SetAxes(int *axes, int dim)
{
  int allAxes[VTK_IMAGE_DIMENSIONS];
  int axesTable[VTK_IMAGE_DIMENSIONS];
  int idx, axis;
  int modifiedFlag = 0;

  
  // Clear the table
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    axesTable[idx] = 0;
    }
  
  // Copy the axes passed as parameters (and add to table)
  for (idx = 0; idx < dim; ++idx)
    {
    axis = axes[idx];
    allAxes[idx] = axis;
    // Error checking
    if (axis < 0 || axis >= VTK_IMAGE_DIMENSIONS)
      {
      vtkErrorMacro(<< "SetAxes: Bad axis: " << axis);
      return;
      }
    // More Error checking
    if (axesTable[axis])
      {
      vtkErrorMacro(<< "SetAxes: Axis " << axis << " occurs more than once");
      return;
      }
    // save axis in table
    axesTable[axis] = 1;
    }
  
  // Set the unspecified axes.
  for (idx = dim; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    // choose the first untaken axis
    axis = 0;
    while (axesTable[axis])
      {
      ++axis;
      }
    allAxes[idx] = axis;
    axesTable[axis] = 1;
    }
  
  // Check the case where the axes are the same
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (this->Axes[idx] != allAxes[idx])
      {
      modifiedFlag = 1;
      break;
      }
    }
  if (! modifiedFlag)
    {
    return;
    }
  
  // Axes have been modified
  this->Modified();
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Axes[idx] = allAxes[idx];
    }
  
  // Any DefaultCoordinates set must be invalid now.
  this->ResetDefaultCoordinates(VTK_IMAGE_DIMENSIONS);
  
  // Recompute relative IVARs
  this->ShuffleBoundsAbsoluteToRelative(this->AbsoluteBounds, this->Bounds);
  this->ShuffleBoundsAbsoluteToRelative(this->AbsoluteImageBounds, 
					this->ImageBounds);
  vtkImageRegionShuffleAbsoluteToRelative(this->AbsoluteAspectRatio, this->AspectRatio,
					  this->Axes);

  if (this->Data)
    {
    vtkImageRegionShuffleAbsoluteToRelative(this->Data->GetIncrements(), 
					    this->Increments, this->Axes);
    }
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
  if (dim > 0) 
    this->DefaultCoordinate0 = this->Bounds[0];
  if (dim > 1) 
    this->DefaultCoordinate1 = this->Bounds[2];
  if (dim > 2) 
    this->DefaultCoordinate2 = this->Bounds[4];
  if (dim > 3)
    this->DefaultCoordinate3 = this->Bounds[6];
  if (dim > 4)
    this->DefaultCoordinate4 = this->Bounds[8];
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
  this->ShuffleBoundsRelativeToAbsolute(this->Bounds, this->AbsoluteBounds);
  this->ResetDefaultCoordinates(dim);
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
  this->ShuffleBoundsRelativeToAbsolute(this->ImageBounds, 
					this->AbsoluteImageBounds);
  this->ResetDefaultCoordinates(dim);
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




//----------------------------------------------------------------------------
void vtkImageRegion::SetAspectRatio(float *ratio, int dim)
{
  int idx;
  
  for (idx = 0; idx < dim; ++idx)
    {
    this->AspectRatio[idx] = ratio[idx];
    }
  
  vtkImageRegionShuffleRelativeToAbsolute(this->AspectRatio, this->AbsoluteAspectRatio,
					  this->Axes);
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetAspectRatio(float *ratio, int dim)
{
  int idx;
  
  for (idx = 0; idx < dim; ++idx)
    {
    ratio[idx] = this->AspectRatio[idx];
    }
}









//----------------------------------------------------------------------------
// since data in region has same bounds as region, 5 nested loops are not
// actually necessary.  But to keep this method tolerent to future changes ...
template <class T>
void vtkImageRegionImportMemory(vtkImageRegion *self, T *memPtr)
{
  int min0, max0, min1, max1, min2, max2, min3, max3, min4, max4;
  int inc0, inc1, inc2, inc3, inc4;
  int idx0, idx1, idx2, idx3, idx4;
  T *ptr0, *ptr1, *ptr2, *ptr3, *ptr4;
  
  self->GetIncrements5d(inc0, inc1, inc2, inc3, inc4);
  self->GetBounds5d(min0,max0, min1,max1, min2,max2, min3,max3, min4,max4);
  
  // loop over 5d space.
  ptr4 = (T *)(self->GetVoidPointer5d());
  for (idx4 = min4; idx4 <= max4; ++idx4)
    {
    ptr3 = ptr4;
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
	    *ptr0 = *memPtr++;
	    ptr0 += inc0;
	    }
	  ptr1 += inc1;
	  }
	ptr2 += inc2;
	}
      ptr3 += inc3;
      }
    ptr4 += inc4;
    }
}

//----------------------------------------------------------------------------
// Description:
// This method will copy your memory into the region.  It is important that
// you SetBounds and SetDataType of this region before this method is called.
void vtkImageRegion::ImportMemory(void *ptr)
{
  // Get rid of old data, and allocate new
  this->Allocate();
  
  switch (this->GetDataType())
    {
    case VTK_IMAGE_FLOAT:
      vtkImageRegionImportMemory(this, (float *)(ptr));
      break;
    case VTK_IMAGE_INT:
      vtkImageRegionImportMemory(this, (int *)(ptr));
      break;
    case VTK_IMAGE_SHORT:
      vtkImageRegionImportMemory(this, (short *)(ptr));
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImageRegionImportMemory(this, (unsigned short *)(ptr));
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImageRegionImportMemory(this, (unsigned char *)(ptr));
      break;
    default:
      vtkErrorMacro(<< "ImportMemory: Cannot handle DataType.");
    }   
}


//----------------------------------------------------------------------------
// This should probably copy the data.
void *vtkImageRegion::ExportMemory()
{
  return (void *)(this->Data->GetVoidPointer());
}





