/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRegion.cxx
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
#include "vtkImageRegion.h"

//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageRegion with no data.
vtkImageRegion::vtkImageRegion()
{
  this->Data = NULL;
  this->ScalarType = VTK_VOID;
  this->Axes[0] = VTK_IMAGE_X_AXIS;
  this->Axes[1] = VTK_IMAGE_Y_AXIS;
  this->Axes[2] = VTK_IMAGE_Z_AXIS;
  this->Axes[3] = VTK_IMAGE_TIME_AXIS;
  this->Axes[4] = VTK_IMAGE_COMPONENT_AXIS;

  this->Increments[0] = this->Increments[1] = this->Increments[2]
    = this->Increments[3] = this->Increments[4] = 0;
  
  // Default memory organization
  this->MemoryOrder[0] = VTK_IMAGE_COMPONENT_AXIS;
  this->MemoryOrder[1] = VTK_IMAGE_X_AXIS;
  this->MemoryOrder[2] = VTK_IMAGE_Y_AXIS;
  this->MemoryOrder[3] = VTK_IMAGE_Z_AXIS;
  this->MemoryOrder[4] = VTK_IMAGE_TIME_AXIS;
  
  this->SetExtent(0,0, 0,0, 0,0, 0,0, 0,0);
  this->SetImageExtent(0,0, 0,0, 0,0, 0,0, 0,0);
  this->SetSpacing(1.0, 1.0, 1.0, 1.0, 0.0);
  this->SetOrigin(0.0, 0.0, 0.0, 0.0, 0.0);
}


//----------------------------------------------------------------------------
// Description:
// Destructor: Deleting a vtkImageRegion automatically deletes the associated
// vtkImageData.  However, since the data is reference counted, it may not 
// actually be deleted.
vtkImageRegion::~vtkImageRegion()
{
  this->ReleaseData();
}


//----------------------------------------------------------------------------
void vtkImageRegion::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkImageSource::PrintSelf(os,indent);
  os << indent << "Axes: (" << vtkImageAxisNameMacro(this->Axes[0]);
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << vtkImageAxisNameMacro(this->Axes[idx]);
    }
  os << ")\n";
  
  os << indent << "MemoryOrder: (" 
     << vtkImageAxisNameMacro(this->MemoryOrder[0]);
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << vtkImageAxisNameMacro(this->MemoryOrder[idx]);
    }
  os << ")\n";
  
  os << indent << "Extent: (" << this->Extent[0];
  for (idx = 1; idx < VTK_IMAGE_EXTENT_DIMENSIONS; ++idx)
    {
    os << ", " << this->Extent[idx];
    }
  os << ")\n";
  
  os << indent << "ImageExtent: (" << this->ImageExtent[0];
  for (idx = 1; idx < VTK_IMAGE_EXTENT_DIMENSIONS; ++idx)
    {
    os << ", " << this->ImageExtent[idx];
    }
  os << ")\n";
  
  os << indent << "Spacing: (" << this->Spacing[0];
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << this->Spacing[idx];
    }
  os << ")\n";
  
  os << indent << "Origin: (" << this->Origin[0];
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << this->Origin[idx];
    }
  os << ")\n";
  
  os << indent << "Increments: (" << this->Increments[0];
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << this->Increments[idx];
    }
  os << ")\n";
  
  os << indent << "ScalarType: " 
     << vtkImageScalarTypeNameMacro(this->ScalarType) << "\n";
  
  if ( ! this->Data)
    {
    os << indent << "Data: NULL\n";
    }
  else
    {
    os << indent << "Data: (" << this->Data << ")\n";
    this->Data->PrintSelf(os,indent.GetNextIndent());
    }
}



  

//----------------------------------------------------------------------------
// Description:
// Convert 4d vector (not extent!) from one coordinate system into another
// coordinate system.  "vectIn" and "vectOut" may be the same array.
template <class T>
static void
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
void vtkImageRegion::ChangeVectorCoordinateSystem(float *vectIn, int *axesIn, 
						  float *vectOut, int *axesOut)
{
  vtkImageRegionChangeVectorCoordinateSystem(vectIn, axesIn, vectOut, axesOut);
}
//----------------------------------------------------------------------------
void vtkImageRegion::ChangeVectorCoordinateSystem(int *vectIn, int *axesIn,
						  int *vectOut, int *axesOut)
{
  vtkImageRegionChangeVectorCoordinateSystem(vectIn, axesIn, vectOut, axesOut);
}

//----------------------------------------------------------------------------
// Description:
// Convert 4d extent from one coordinate system into another.
// "extentIn" and "extentOut" may be the same array.
void vtkImageRegion::ChangeExtentCoordinateSystem(int *extentIn, int *axesIn,
						  int *extentOut, int *axesOut)
{
  int idx;
  int absolute[VTK_IMAGE_EXTENT_DIMENSIONS];

  // Change into a known coordinate system (0,1,2,...)
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    absolute[axesIn[idx]*2] = extentIn[idx*2];
    absolute[axesIn[idx]*2+1] = extentIn[idx*2+1];
    }

  // Change into the desired coordinate system.
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    extentOut[idx*2] = absolute[axesOut[idx]*2];
    extentOut[idx*2+1] = absolute[axesOut[idx]*2+1];
    }
}

//----------------------------------------------------------------------------
// Description:
// This method determines the order of unspecified axes.  By default
// lower enumerated axes come first.  This method is used by
// vtkImageRegion and vtkImageData.
void vtkImageRegion::CompleteUnspecifiedAxes(int num, int *axesIn, 
					     int *axesOut)
{
  int axesTable[VTK_IMAGE_DIMENSIONS];
  int idx, axis;
  
  // Clear the table
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    axesTable[idx] = 0;
    }
  
  // Copy the axes passed as parameters (and add to table)
  for (idx = 0; idx < num; ++idx)
    {
    axis = axesIn[idx];
    axesOut[idx] = axis;
    // Error checking
    if (axis < 0 || axis >= VTK_IMAGE_DIMENSIONS)
      {
      vtkGenericWarningMacro("CompleteUnspecifiedAxes: Bad axis: " << axis);
      return;
      }
    // More Error checking
    if (axesTable[axis])
      {
      vtkGenericWarningMacro("CompleteUnspecifiedAxes: Axis " << axis 
        << " occurs more than once");
      return;
      }
    // save axis in table
    axesTable[axis] = 1;
    }
  
  // Set the unspecified axes.
  for (idx = num; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    // choose the first untaken axis
    axis = 0;
    while (axesTable[axis])
      {
      ++axis;
      }
    axesOut[idx] = axis;
    axesTable[axis] = 1;
    }
}

//----------------------------------------------------------------------------
// Description:
// This function makes sure we are the only one referenceing the data.
// The data object is copied if necessary.  It does not make the point
// data writable.
void vtkImageRegion::MakeDataWritable()
{
  // Check to make sure we have a data object.
  if ( ! this->Data)
    {
    this->Modified();
    this->MakeData();
    }

  // Check to make sure no one is referencing the data object.
  if (this->Data->GetReferenceCount() > 1)
    {
    vtkImageData *newData;
    vtkScalars *scalars = this->Data->GetPointData()->GetScalars();
    // Data has more than one reference. Make a new data object.
    this->Modified();
    newData = vtkImageData::New();
    newData->SetAxes(this->Data->GetAxes());
    newData->SetExtent(this->Data->GetExtent());
    // But what if the scalars have more than one reference?
    newData->GetPointData()->SetScalars(scalars);
    this->Data->UnRegister(this);
    this->Data = newData;
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
  region->ReleaseData();
  region->SetScalarType(this->GetScalarType());
  region->SetData(this->GetData());
}

  
//----------------------------------------------------------------------------
// Description:
// Returns the extent of the region as the image extent.
void vtkImageRegion::UpdateImageInformation(vtkImageRegion *region)
{
  int axesSave[VTK_IMAGE_DIMENSIONS];
  
  // Save coordinate system
  region->GetAxes(axesSave);
  // convert to this regions coordinate system
  region->SetAxes(this->GetAxes());
  // Set the extent
  region->SetImageExtent(this->GetExtent());
  // Set the data spacing
  region->SetSpacing(this->GetSpacing());
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




//----------------------------------------------------------------------------
// Description:
// When dealing with Regions directly (no caches), they can be allocated
// with this method.  It keeps you from having to create a vtkImageData
// object and setting it explicitley.
void vtkImageRegion::AllocateScalars()
{
  this->MakeScalarsWritable();
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

  this->ScalarType = VTK_VOID;
}


//----------------------------------------------------------------------------
// Description:
// These method return the increments between pixels, rows, images and volumes.
// A Coordinate system relative to "Axes" is used to set the order.
// These values are determined by the actual dimensions of the data stored
// in the vtkImageData object.  "Increments" allows the user to efficiently 
// march through the memory using pointer arithmetic, while keeping the
// actual dimensions of the memory array transparent.  
void vtkImageRegion::GetIncrements(int dim, int *increments)
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
void *vtkImageRegion::GetScalarPointer(int dim, int *coordinates)
{
  int idx, temp[VTK_IMAGE_DIMENSIONS];
  
  if ( ! this->Data)
    {
    // Create the data object
    // Note: This is only call if the data does not exists.
    // There are no reference counting issues.
    this->MakeDataWritable();
    }

  // Copy coordinates.
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (idx < dim)
      {
      temp[idx] = coordinates[idx];
      }
    else
      {
      temp[idx] = this->Extent[idx * 2];
      }
    }
  
  // Convert into data coordinates
  vtkImageRegionChangeVectorCoordinateSystem(temp, this->Axes,
					     temp, this->Data->GetAxes());
  
  return this->Data->GetScalarPointer(temp);
}



//----------------------------------------------------------------------------
void vtkImageRegion::SetAxes(int num, int *axes)
{
  int allAxes[VTK_IMAGE_DIMENSIONS];
  int idx;
  int modifiedFlag = 0;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro("SetAxes: " << num << " is too many");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  // Check the case where the axes are the same
  for (idx = 0; idx < num; ++idx)
    {
    if (this->Axes[idx] != axes[idx])
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
  
  // Get the complete axes (all 5)
  vtkImageRegion::CompleteUnspecifiedAxes(num, axes, allAxes);
  
  // Change the coordinate system of the ivars.
  vtkImageRegion::ChangeVectorCoordinateSystem(this->Spacing, this->Axes,
					       this->Spacing, allAxes);
  vtkImageRegion::ChangeVectorCoordinateSystem(this->Origin, this->Axes,
					       this->Origin, allAxes);
  vtkImageRegion::ChangeVectorCoordinateSystem(this->Increments, this->Axes,
					       this->Increments, allAxes);
  vtkImageRegion::ChangeExtentCoordinateSystem(this->Extent, this->Axes,
					       this->Extent, allAxes);
  vtkImageRegion::ChangeExtentCoordinateSystem(this->ImageExtent, this->Axes,
					       this->ImageExtent, allAxes);

  // Actually change the regions axes.
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Axes[idx] = allAxes[idx];
    }
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetAxes(int num, int *axes)
{
  int idx;
  
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro("GetAxes: Too many axes requested");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    axes[idx] = this->Axes[idx];
    }
}





//----------------------------------------------------------------------------
void vtkImageRegion::SetExtent(int dim, int *extent)
{
  int idx;
  int extentDim = dim * 2;
  
  for (idx = 0; idx < extentDim; ++idx)
    {
    this->Extent[idx] = extent[idx];
    }
  this->Modified();
  
  // also set the extent of the data if it has not been allocated.
  if (this->Data && ( ! this->Data->AreScalarsAllocated()))
    {
    int saveAxes[VTK_IMAGE_DIMENSIONS];
    this->GetAxes(VTK_IMAGE_DIMENSIONS, saveAxes);
    this->SetAxes(VTK_IMAGE_DIMENSIONS, this->MemoryOrder);
    this->Data->SetExtent(dim, this->Extent);
    this->Data->GetIncrements(VTK_IMAGE_DIMENSIONS, this->Increments);
    this->SetAxes(VTK_IMAGE_DIMENSIONS, saveAxes);
    }
}

//----------------------------------------------------------------------------
void vtkImageRegion::GetExtent(int dim, int *extent)
{
  int idx;
  
  dim *= 2;
  for (idx = 0; idx < dim; ++idx)
    {
    extent[idx] = this->Extent[idx];
    }
}




//----------------------------------------------------------------------------
void vtkImageRegion::SetImageExtent(int dim, int *extent)
{
  int idx;
  int extentDim = dim * 2;
  
  for (idx = 0; idx < extentDim; ++idx)
    {
    this->ImageExtent[idx] = extent[idx];
    }  
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetImageExtent(int dim, int *boundary)
{
  int idx;
  
  dim *= 2;
  for (idx = 0; idx < dim; ++idx)
    {
    boundary[idx] = this->ImageExtent[idx];
    }
}






//----------------------------------------------------------------------------
void vtkImageRegion::SetSpacing(int dim, float *ratio)
{
  int idx;
  
  for (idx = 0; idx < dim; ++idx)
    {
    this->Spacing[idx] = ratio[idx];
    }
  
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetSpacing(int dim, float *ratio)
{
  int idx;
  
  for (idx = 0; idx < dim; ++idx)
    {
    ratio[idx] = this->Spacing[idx];
    }
}




//----------------------------------------------------------------------------
void vtkImageRegion::SetOrigin(int dim, float *ratio)
{
  int idx;
  
  for (idx = 0; idx < dim; ++idx)
    {
    this->Origin[idx] = ratio[idx];
    }
  
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageRegion::GetOrigin(int dim, float *ratio)
{
  int idx;
  
  for (idx = 0; idx < dim; ++idx)
    {
    ratio[idx] = this->Origin[idx];
    }
}



//----------------------------------------------------------------------------
void vtkImageRegion::Translate(int dim, int *vector)
{
  int idx;
  vtkImageData *newData;
  int allVector[VTK_IMAGE_DIMENSIONS];
  
  // Change extent and image extent of this region.
  for (idx = 0; idx < dim; ++idx)
    {
    this->Extent[idx*2] += vector[idx];
    this->Extent[1+idx*2] += vector[idx];
    this->ImageExtent[idx*2] += vector[idx];
    this->ImageExtent[1+idx*2] += vector[idx];
    }

  // Since the data might have multiple references, we can not just modify it.
  if (this->Data && this->Data->GetReferenceCount() > 1)
    {
    newData = vtkImageData::New();
    newData->SetAxes(this->Data->GetAxes());
    newData->SetExtent(this->Data->GetExtent());
    newData->SetScalars(this->Data->GetPointData()->GetScalars());
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


  
  





/*****************************************************************************
  Stuff for copying data (double templated).
  Convenience Methods.
*****************************************************************************/

  
//----------------------------------------------------------------------------
// Second templated function for copying.
// This should be a recursive call to avoid many nested loops, and
// make the code independant of VTK_IMAGE_DIMENSIONS. (In my dreams...)
template <class IT, class OT>
static void vtkImageRegionCopyData2(vtkImageRegion *self, OT *outPtr,
				    vtkImageRegion *in, IT *inPtr)
{
  IT *inPtr0, *inPtr1, *inPtr2, *inPtr3, *inPtr4;
  OT *outPtr0, *outPtr1, *outPtr2, *outPtr3, *outPtr4;
  int inInc0, inInc1, inInc2, inInc3, inInc4;
  int outInc0, outInc1, outInc2, outInc3, outInc4;
  int min0, max0, min1, max1, min2, max2, min3, max3, min4, max4;
  int idx0, idx1, idx2, idx3, idx4;

  // Get information to loop through data.
  self->GetExtent(min0, max0, min1, max1, min2, max2, min3, max3, min4, max4);
  self->GetIncrements(outInc0, outInc1, outInc2, outInc3, outInc4);
  in->GetIncrements(inInc0, inInc1, inInc2, inInc3, inInc4);
  
  inPtr4 = inPtr;
  outPtr4 = outPtr;
  for (idx4 = min4; idx4 <= max4; ++idx4)
    {
    inPtr3 = inPtr4;
    outPtr3 = outPtr4;
    for (idx3 = min3; idx3 <= max3; ++idx3)
      {
      inPtr2 = inPtr3;
      outPtr2 = outPtr3;
      for (idx2 = min2; idx2 <= max2; ++idx2)
	{
	inPtr1 = inPtr2;
	outPtr1 = outPtr2;
	for (idx1 = min1; idx1 <= max1; ++idx1)
	  {
	  inPtr0 = inPtr1;
	  outPtr0 = outPtr1;
	  for (idx0 = min0; idx0 <= max0; ++idx0)
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
static void vtkImageRegionCopyData(vtkImageRegion *self, vtkImageRegion *in,
				   T * inPtr)
{
  void *outPtr = self->GetScalarPointer();
  
  switch (self->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageRegionCopyData2(self, (float *)(outPtr), in, inPtr);
      break;
    case VTK_INT:
      vtkImageRegionCopyData2(self, (int *)(outPtr), in, inPtr);
      break;
    case VTK_SHORT:
      vtkImageRegionCopyData2(self, (short *)(outPtr), in, inPtr);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageRegionCopyData2(self, (unsigned short *)(outPtr), in, inPtr);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageRegionCopyData2(self, (unsigned char *)(outPtr), in, inPtr);
      break;
    default:
      vtkGenericWarningMacro(
       "vtkImageRegionCopyData2: Cannot handle ScalarType.");
    }   
}

  



//----------------------------------------------------------------------------
// Description:
// Copies data from a region into this region (converting data type).
// It is a simple cast, and will not deal with float to unsigned char
// inteligently.  If regions do not have the same extent,
// the intersection is copied.  The coordinate systems (Axes) of the 
// two regions are important.  This method can be used to switch the 
// labeling of the axes (rather inefficiently).
void vtkImageRegion::CopyRegionData(vtkImageRegion *region)
{
  int thisExtentSave[VTK_IMAGE_EXTENT_DIMENSIONS];
  int regionExtentSave[VTK_IMAGE_EXTENT_DIMENSIONS];  
  int overlap[VTK_IMAGE_EXTENT_DIMENSIONS];
  int idx, inTemp, outTemp;
  void *inPtr;

  // If the data type is not set, default to same as input.
  if (this->GetScalarType() == VTK_VOID)
    {
    this->SetScalarType(region->GetScalarType());
    }
  
  // Compute intersection of extent
  this->GetExtent(VTK_IMAGE_DIMENSIONS, thisExtentSave);
  region->GetExtent(VTK_IMAGE_DIMENSIONS, regionExtentSave);
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    inTemp = regionExtentSave[2*idx];
    outTemp = thisExtentSave[2*idx];
    overlap[2*idx] = (inTemp > outTemp) ? inTemp : outTemp;  // Max
    inTemp = regionExtentSave[2*idx + 1];
    outTemp = thisExtentSave[2*idx + 1];
    overlap[2*idx + 1] = (inTemp < outTemp) ? inTemp : outTemp;  // Min
    }
  
  // Copy data
  this->SetExtent(VTK_IMAGE_DIMENSIONS, overlap);
  region->SetExtent(VTK_IMAGE_DIMENSIONS, overlap);
  inPtr = region->GetScalarPointer();
  switch (region->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageRegionCopyData(this, region, (float *)(inPtr));
      break;
    case VTK_INT:
      vtkImageRegionCopyData(this, region ,(int *)(inPtr));
      break;
    case VTK_SHORT:
      vtkImageRegionCopyData(this, region, (short *)(inPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageRegionCopyData(this, region, (unsigned short *)(inPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageRegionCopyData(this, region, (unsigned char *)(inPtr));
      break;
    default:
      vtkErrorMacro(<< "CopyRegionData: Cannot Handle Input Type.");
    }

  // restore the original extents of the regions
  this->SetExtent(VTK_IMAGE_DIMENSIONS, thisExtentSave);
  region->SetExtent(VTK_IMAGE_DIMENSIONS, regionExtentSave);
}


//****************************************************************************
// Stuff for filling a region with a constant value.
// Convenience method.
//****************************************************************************

//----------------------------------------------------------------------------
template <class T>
static void vtkImageRegionFill(vtkImageRegion *self, T value)
{
  int min0, max0, min1, max1, min2, max2, min3, max3, min4, max4;
  int inc0, inc1, inc2, inc3, inc4;
  int idx0, idx1, idx2, idx3, idx4;
  T *ptr0, *ptr1, *ptr2, *ptr3, *ptr4;
  
  ptr4 = (T *)(self->GetScalarPointer());
  self->GetIncrements(inc0, inc1, inc2, inc3, inc4);
  self->GetExtent(min0,max0, min1,max1, min2,max2, min3,max3, min4,max4);
  
  // loop over 5d space.
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
	    *ptr0 = value;
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
// This function sets all the pixels in a region to the specified value.
void vtkImageRegion::Fill(float value)
{
  this->Modified();
  
  switch (this->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageRegionFill(this, (float)(value));
      break;
    case VTK_INT:
      vtkImageRegionFill(this, (int)(value));
      break;
    case VTK_SHORT:
      vtkImageRegionFill(this, (short)(value));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageRegionFill(this, (unsigned short)(value));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageRegionFill(this, (unsigned char)(value));
      break;
    default:
      vtkErrorMacro(<< "Fill: Cannot handle ScalarType.");
    }   
}


//----------------------------------------------------------------------------
// Description:
// Sets the scalar type of the region.  The region should not be allocated
// yet.
void vtkImageRegion::SetScalarType(int type)
{
  if (this->ScalarType == type)
    {
    return;
    }
  
  if (this->Data)
    {
    if (this->Data->AreScalarsAllocated())
      {
      vtkErrorMacro("SetScalarType: Data has already been allocated.");
      return;
      }
    this->Data->SetScalarType(type);
    }
  
  this->Modified();
  this->ScalarType = type;
}


//----------------------------------------------------------------------------
void vtkImageRegion::SetMemoryOrder(int num, int *order)
{
  if (this->Data && this->Data->AreScalarsAllocated())
    {
    vtkErrorMacro("SetMemoryOrder: Data has already been allocated.");
    return;
    }
  
  this->Modified();
  vtkImageRegion::CompleteUnspecifiedAxes(num, order, this->MemoryOrder);
  if (this->Data)
    {
    this->Data->SetAxes(VTK_IMAGE_DIMENSIONS, this->MemoryOrder);
    }
}

//----------------------------------------------------------------------------
void vtkImageRegion::GetMemoryOrder(int num, int *order)
{
  int idx;
  
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro("GetMemoryOrder: " << num << " axes is too many");
    num = VTK_IMAGE_DIMENSIONS;
    }

  for (idx = 0; idx < num; ++idx)
    {
    order[idx] = this->MemoryOrder[idx];
    }
}



  
//----------------------------------------------------------------------------
// Description:
// If the data of this region is not going to be set explicitely, this
// method can be used to make a new data object for the region.
// Extent, ScalarType, and MemoryOrder should be set before this method
// is called.
void vtkImageRegion::MakeData()
{
  int saveAxes[VTK_IMAGE_DIMENSIONS];
  
  if (this->Data)
    {
    vtkWarningMacro("MakeData: Data object already exists.");
    return;
    }

  this->GetAxes(VTK_IMAGE_DIMENSIONS, saveAxes);
  // change to data coordinates
  this->SetAxes(VTK_IMAGE_DIMENSIONS, this->MemoryOrder);
  
  this->Data = vtkImageData::New();
  this->Data->SetScalarType(this->ScalarType);
  this->Data->SetAxes(VTK_IMAGE_DIMENSIONS, this->MemoryOrder);
  this->Data->SetExtent(VTK_IMAGE_DIMENSIONS, this->Extent);
  this->Data->GetIncrements(VTK_IMAGE_DIMENSIONS, this->Increments);

  // Now restore previous coordinate system
  this->SetAxes(VTK_IMAGE_DIMENSIONS, saveAxes);
}

  

//----------------------------------------------------------------------------
// Description:
// You can set the data object explicitly.
// Old data is released, and the region automatically registers
// the new data.  This assumes that the Data being set 
// has already been allocated.  This method is used mainly by
// vtkImageCache.
void vtkImageRegion::SetData(vtkImageData *data)
{
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
  
  // Set the scalar type
  this->ScalarType = data->GetScalarType();
  
  // set the MemoryOrder of this region
  data->GetAxes(VTK_IMAGE_DIMENSIONS, this->MemoryOrder);
  
  // Compute the increments.
  // Note that this implies that the extent of the data is fixed.
  vtkImageRegionChangeVectorCoordinateSystem(this->Data->GetIncrements(),
					     this->Data->GetAxes(), 
					     this->Increments, this->Axes);
}


//----------------------------------------------------------------------------
// Description:
// Returns the data.  If one does not exist, the a data object is created.
vtkImageData *vtkImageRegion::GetData()
{
  if ( ! this->Data)
    {
    this->MakeData();
    }
  return this->Data;
}



