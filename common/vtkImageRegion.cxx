/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRegion.cxx
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
#include "vtkImageRegion.h"

//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageRegion with no data.
vtkImageRegion::vtkImageRegion()
{
  this->Data = NULL;
  this->DataType = VTK_IMAGE_VOID;
  this->Axes[0] = VTK_IMAGE_X_AXIS;
  this->Axes[1] = VTK_IMAGE_Y_AXIS;
  this->Axes[2] = VTK_IMAGE_Z_AXIS;
  this->Axes[3] = VTK_IMAGE_TIME_AXIS;
  this->Axes[4] = VTK_IMAGE_COMPONENT_AXIS;

  this->Increments[0] = this->Increments[1] = this->Increments[2]
    = this->Increments[3] = this->Increments[4] = 0;
  
  this->SetBounds5d(0,0, 0,0, 0,0, 0,0, 0,0);
  this->SetImageBounds5d(0,0, 0,0, 0,0, 0,0, 0,0);
  this->ResetDefaultCoordinates(5);
  this->SetAspectRatio5d(0.0, 0.0, 0.0, 0.0, 0.0);
  this->SetOrigin5d(0.0, 0.0, 0.0, 0.0, 0.0);
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
void vtkImageRegion::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);
  os << indent << "Axes: (";
  os << vtkImageAxisNameMacro(this->Axes[0]) << ", ";
  os << vtkImageAxisNameMacro(this->Axes[1]) << ", ";
  os << vtkImageAxisNameMacro(this->Axes[2]) << ", ";
  os << vtkImageAxisNameMacro(this->Axes[3]) << ")\n";
  
  os << indent << "Bounds: (";
  os << this->Bounds[0] << ", " << this->Bounds[1] << ", ";
  os << this->Bounds[2] << ", " << this->Bounds[3] << ", ";
  os << this->Bounds[4] << ", " << this->Bounds[5] << ", ";
  os << this->Bounds[6] << ", " << this->Bounds[7] << ")\n";
  
  os << indent << "Default Coordinates: (";
  os << this->DefaultCoordinates[0] << ", ";
  os << this->DefaultCoordinates[1] << ", ";
  os << this->DefaultCoordinates[2] << ", ";
  os << this->DefaultCoordinates[3] << ")\n";
  
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

  os << indent << "Origin: (";
  os << this->Origin[0] << ", ";
  os << this->Origin[1] << ", ";
  os << this->Origin[2] << ", ";
  os << this->Origin[3] << ")\n";

  os << indent << "DataType: " << vtkImageDataTypeNameMacro(this->DataType) 
     << "\n";
  
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


//----------------------------------------------------------------------------
// Description:
// Returns the size of the bounded memory in KiloBytes.  
// It is used to determine when to split while streaming.
int vtkImageRegion::GetMemorySize()
{
  int size;
  
  switch (this->GetDataType())
    {
    case VTK_IMAGE_FLOAT:
      size = sizeof(float);
      break;
    case VTK_IMAGE_INT:
      size = sizeof(int);
      break;
    case VTK_IMAGE_SHORT:
      size = sizeof(short);
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      size = sizeof(unsigned short);
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      size = sizeof(char);
      break;
    default:
      vtkErrorMacro(<< "ImportMemory: Cannot handle DataType.");
    }   
  
  return size * this->GetVolume() / 1000;
}



//----------------------------------------------------------------------------
// Description:
// This function allows you to write into a region.  If there are multple
// references to the data, the data is copied into a new object.
void vtkImageRegion::MakeWritable()
{
  vtkImageData *newData;
  int axesSave[VTK_IMAGE_DIMENSIONS];
  
  if ((this->Data->GetRefCount() > 1) || 
      (this->Data->GetScalars()->GetRefCount() > 1))
    {
    vtkDebugMacro(<< "MakeWritable: Need to copy data because of references.");
    newData = new vtkImageData;
    newData->SetAxes(this->Data->GetAxes());
    
    // Set the bounds (the same as region)
    this->GetAxes(axesSave);
    this->SetAxes(newData->GetAxes());
    newData->SetBounds(this->GetBounds());
    this->SetAxes(axesSave);
    
    // Replace the data object with a copy
    newData->CopyData(this->Data);
    this->SetData(newData);
    newData->Delete();
    }
}



//----------------------------------------------------------------------------
// Description:
// Copies data from a region into this region (converting data type).
// It is a simple cast, and will not deal with float to unsigned char
// inteligently.  The the regions do not have the same bounds,
// the intersection is copied.
void vtkImageRegion::CopyRegionData(vtkImageRegion *region)
{
  int thisAxesSave[VTK_IMAGE_DIMENSIONS];
  int regionAxesSave[VTK_IMAGE_DIMENSIONS];  
  int overlap[VTK_IMAGE_BOUNDS_DIMENSIONS];
  int *inBounds, *outBounds;
  int inTemp, outTemp;
  int idx;

  // Make sure this region is allocated
  if ( ! this->IsAllocated())
    {
    this->Allocate();
    }
  if ( ! this->IsAllocated())
    {
    vtkErrorMacro(<< "Could not allocate region.");
    return;
    }

  // Convert to common coordinate system of data.
  this->GetAxes(thisAxesSave);
  region->GetAxes(regionAxesSave);
  this->SetAxes(this->Data->GetAxes());
  region->SetAxes(this->Data->GetAxes());
  
  // Compute intersection of bounds
  inBounds = region->GetBounds();
  outBounds = this->GetBounds();
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    inTemp = inBounds[2*idx];
    outTemp = outBounds[2*idx];
    overlap[2*idx] = (inTemp > outTemp) ? inTemp : outTemp;  // Max
    inTemp = inBounds[2*idx + 1];
    outTemp = outBounds[2*idx + 1];
    overlap[2*idx + 1] = (inTemp < outTemp) ? inTemp : outTemp;  // Min
    }
  
  // If the data type is not set, default to same as input.
  if (this->GetDataType() == VTK_IMAGE_VOID)
    {
    this->SetDataType(region->GetDataType());
    }
  
  // Copy data
  this->Data->CopyData(region->GetData(), overlap);

  // restore the original coordinate system of the regions.
  this->SetAxes(thisAxesSave);
  region->SetAxes(regionAxesSave);
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
  region->ReleaseData();
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
  // Set the aspect Ratio
  region->SetAspectRatio(this->GetAspectRatio());
  // Set the origin
  region->SetOrigin(this->GetOrigin());
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
// Convert 4d vector (not bounds!) from one coordinate system into another
// coordinate system.  "vectIn" and "vectOut" may be the same array.
template <class T>
void
vtkImageRegionChangeVectorCoordinateSystem(T *vectIn, int *axesIn, 
					   T *vectOut, int *axesOut)
{
  int idx;
  T absolute[VTK_IMAGE_DIMENSIONS];

  // Convert to an intermediate coordinate system (0,1,2,...)
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    absolute[axesIn[idx]] = vectIn[idx];
    }
  // Change back into the new coordinate system
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    vectOut[idx] = absolute[axesOut[idx]];
    }
}


//----------------------------------------------------------------------------
// Description:
// Convert 4d bounds from one coordinate system into another.
// "boundsIn" and "boundsOut" may be the same array.
void vtkImageRegion::ChangeBoundsCoordinateSystem(int *boundsIn, int *axesIn,
						  int *boundsOut, int *axesOut)
{
  int idx;
  int absolute[VTK_IMAGE_BOUNDS_DIMENSIONS];

  // Change into a known coordinate system (0,1,2,...)
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    absolute[axesIn[idx]*2] = boundsIn[idx*2];
    absolute[axesIn[idx]*2+1] = boundsIn[idx*2+1];
    }

  // Change into the desired coordinate system.
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    boundsOut[idx*2] = absolute[axesOut[idx]*2];
    boundsOut[idx*2+1] = absolute[axesOut[idx]*2+1];
    }
}



//----------------------------------------------------------------------------
// Description:
// When dealing with Regions directly (no caches), they can be allocated
// with this method.  It keeps you from having to create a vtkImageData
// object and setting it explicitley.
void vtkImageRegion::Allocate()
{
  int bounds[VTK_IMAGE_BOUNDS_DIMENSIONS];
  
  this->Modified();

  if (this->Data)
    {
    this->Data->Delete();
    this->Data = NULL;
    }

  this->Data = new vtkImageData;
  this->Data->SetType(this->DataType);
  this->GetBounds(bounds);
  this->ChangeBoundsCoordinateSystem(this->Bounds, this->Axes,
				     bounds, this->Data->GetAxes());
  this->Data->SetBounds(bounds);
  this->Data->Allocate();
  
  // Compute the increments.
  vtkImageRegionChangeVectorCoordinateSystem(this->Data->GetIncrements(),
					     this->Data->GetAxes(),
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
  
  // Compute the increments.
  vtkImageRegionChangeVectorCoordinateSystem(this->Data->GetIncrements(),
					     this->Data->GetAxes(),
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
// of the location are in pixel units and are relative to the
// origin of the whole image. The region just forwards the message
// to its vtkImageData object.
void *vtkImageRegion::GetVoidPointer5d(int coordinates[5])
{
  int temp[VTK_IMAGE_DIMENSIONS];
  
  if ( ! this->Data){
    vtkErrorMacro(<<"Data must be set or allocated.");
    return NULL;
  }

  // Convert into data coordinates
  vtkImageRegionChangeVectorCoordinateSystem(coordinates, this->Axes,
					     temp, this->Data->GetAxes());
  
  return this->Data->GetVoidPointer(temp);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer4d(int coordinates[4])
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = coordinates[0];
  coords[1] = coordinates[1];
  coords[2] = coordinates[2];
  coords[3] = coordinates[3];
  coords[4] = this->DefaultCoordinates[4];
  
  return this->GetVoidPointer5d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer3d(int coordinates[3])
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = coordinates[0];
  coords[1] = coordinates[1];
  coords[2] = coordinates[2];
  coords[3] = this->DefaultCoordinates[3];
  coords[4] = this->DefaultCoordinates[4];
  
  return this->GetVoidPointer5d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer2d(int coordinates[2])
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = coordinates[0];
  coords[1] = coordinates[1];
  coords[2] = this->DefaultCoordinates[2];
  coords[3] = this->DefaultCoordinates[3];
  coords[4] = this->DefaultCoordinates[4];
  
  return this->GetVoidPointer5d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer1d(int coordinates[1])
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = coordinates[0];
  coords[1] = this->DefaultCoordinates[1];
  coords[2] = this->DefaultCoordinates[2];
  coords[3] = this->DefaultCoordinates[3];
  coords[4] = this->DefaultCoordinates[4];
  
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
  coords[4] = this->DefaultCoordinates[4];
  
  return this->GetVoidPointer5d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer3d()
{
  int coords[VTK_IMAGE_DIMENSIONS];

  coords[0] = this->Bounds[0];
  coords[1] = this->Bounds[2];
  coords[2] = this->Bounds[4];
  coords[3] = this->DefaultCoordinates[3];
  coords[4] = this->DefaultCoordinates[4];
  
  return this->GetVoidPointer5d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer2d()
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = this->Bounds[0];
  coords[1] = this->Bounds[2];
  coords[2] = this->DefaultCoordinates[2];
  coords[3] = this->DefaultCoordinates[3];
  coords[4] = this->DefaultCoordinates[4];
  
  return this->GetVoidPointer5d(coords);
}
//----------------------------------------------------------------------------
void *vtkImageRegion::GetVoidPointer1d()
{
  int coords[VTK_IMAGE_DIMENSIONS];
  
  coords[0] = this->Bounds[0];
  coords[1] = this->DefaultCoordinates[1];
  coords[2] = this->DefaultCoordinates[2];
  coords[3] = this->DefaultCoordinates[3];
  coords[4] = this->DefaultCoordinates[4];
  
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
  
  // Change the coordinate system of the ivars.
  vtkImageRegionChangeVectorCoordinateSystem(
				     this->DefaultCoordinates, this->Axes,
				     this->DefaultCoordinates, allAxes);
  vtkImageRegionChangeVectorCoordinateSystem(this->AspectRatio, this->Axes,
					     this->AspectRatio, allAxes);
  vtkImageRegionChangeVectorCoordinateSystem(this->Origin, this->Axes,
					     this->Origin, allAxes);
  vtkImageRegionChangeVectorCoordinateSystem(this->Increments, this->Axes,
					     this->Increments, allAxes);
  this->ChangeBoundsCoordinateSystem(this->Bounds, this->Axes,
				     this->Bounds, allAxes);
  this->ChangeBoundsCoordinateSystem(this->ImageBounds, this->Axes,
				     this->ImageBounds, allAxes);

  // Actually change the regions axes.
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Axes[idx] = allAxes[idx];
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
    this->DefaultCoordinates[0] = this->Bounds[0];
  if (dim > 1) 
    this->DefaultCoordinates[1] = this->Bounds[2];
  if (dim > 2) 
    this->DefaultCoordinates[2] = this->Bounds[4];
  if (dim > 3)
    this->DefaultCoordinates[3] = this->Bounds[6];
  if (dim > 4)
    this->DefaultCoordinates[4] = this->Bounds[8];
}






//----------------------------------------------------------------------------
// Description:
// These methods set the bounds of the region.  Don't forget that
// bounds are relative to the coordinate system of the region (Axes).
void vtkImageRegion::SetBounds(int *bounds, int dim)
{
  int idx;
  int boundsDim = dim * 2;
  
  for (idx = 0; idx < boundsDim; ++idx)
    {
    this->Bounds[idx] = bounds[idx];
    }
  this->ResetDefaultCoordinates(dim);
  this->Modified();
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
// These methods set the image bounds of the region.  Don't forget that
// image bounds are relative to the coordinate system of the region (Axes).
void vtkImageRegion::SetImageBounds(int *bounds, int dim)
{
  int idx;
  int boundsDim = dim * 2;
  
  for (idx = 0; idx < boundsDim; ++idx)
    {
    this->ImageBounds[idx] = bounds[idx];
    }  
  this->ResetDefaultCoordinates(dim);
  this->Modified();
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
  
  this->Modified();
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
void vtkImageRegion::SetOrigin(float *ratio, int dim)
{
  int idx;
  
  for (idx = 0; idx < dim; ++idx)
    {
    this->Origin[idx] = ratio[idx];
    }
  
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetOrigin(float *ratio, int dim)
{
  int idx;
  
  for (idx = 0; idx < dim; ++idx)
    {
    ratio[idx] = this->Origin[idx];
    }
}



//----------------------------------------------------------------------------
void vtkImageRegion::Translate(int *vector, int dim)
{
  int idx;
  vtkImageData *newData;
  int allVector[VTK_IMAGE_DIMENSIONS];
  
  // Change bounds and image bounds of this region.
  for (idx = 0; idx < dim; ++idx)
    {
    this->Bounds[idx*2] += vector[idx];
    this->Bounds[1+idx*2] += vector[idx];
    this->ImageBounds[idx*2] += vector[idx];
    this->ImageBounds[1+idx*2] += vector[idx];
    }

  // Since the data might have multiple references, we can not just modify it.
  if (this->Data && this->Data->GetRefCount() > 1)
    {
    newData = new vtkImageData;
    newData->SetAxes(this->Data->GetAxes());
    newData->SetBounds(this->Data->GetBounds());
    newData->SetScalars(this->Data->GetScalars());
    this->Data->UnRegister(this);
    this->Data = newData;
    }
  
  // Translate data 
  // Change coordinate system of vector to data coordinate system.
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (idx < dim)
      {
      allVector[idx] = vector[idx];
      }
    else
      {
      allVector[idx] = 0;
      }
    }

  if (this->Data)
    {
    vtkImageRegionChangeVectorCoordinateSystem(allVector, this->Axes, 
					     allVector, this->Data->GetAxes());
    this->Data->Translate(allVector);
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
  this->Modified();
  
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





