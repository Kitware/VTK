/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageData.cxx
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
#include "vtkImageData.h"

//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageData with no data.
vtkImageData::vtkImageData()
{
  int idx;

  this->ScalarsAllocated = 0;
  this->ScalarType = VTK_VOID;
  this->PrintScalars = 0;
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Axes[idx] = idx;
    this->Increments[idx] = 0;
    this->Extent[idx*2] = 0;
    this->Extent[idx*2 + 1] = 0;
    }
}


//----------------------------------------------------------------------------
// A templated function to print different types of pointData.
template <class T>
static void vtkImageDataPrintScalars(vtkImageData *self, T *ptr,
				ostream& os, vtkIndent indent)
{
  int precisionSave = os.precision();
  int *temp;
  int idx0, idx1, idx2, idx3;
  int inc0, inc1, inc2, inc3;
  int min0, max0, min1, max1, min2, max2, min3, max3;
  T *ptr0, *ptr1, *ptr2, *ptr3;
  vtkIndent indent0, indent1, indent2, indent3;

  // Only print float values to 2 decimals
  os.precision(2);
  
  self->GetIncrements(inc0, inc1, inc2, inc3);
  self->GetExtent(min0, max0, min1, max1, min2, max2, min3, max3);
  temp = self->GetAxes();
  
  indent3 = indent;
  ptr3 = ptr;
  if (max3 > min3)
    {
    os << indent3 << vtkImageAxisNameMacro(temp[3]) 
       << " range:(" << min3 << ", " << max3 << "), coordinant: (0, 0, 0, 0"
       << ")###########################\n";
    }
  for (idx3 = min3; idx3 <= max3; ++idx3)
    {
    indent2 = indent3.GetNextIndent();
    ptr2 = ptr3;
    if (max2 > min2)
      {
      os << indent2 << vtkImageAxisNameMacro(temp[2]) 
	 << " range:(" << min2 << ", " << max2 << "), coordinant: (0, 0, 0, "
	 << ", " << idx3 << ")===========================\n";
      }
    for (idx2 = min2; idx2 <= max2; ++idx2)
      {
      indent1 = indent2.GetNextIndent();
      ptr1 = ptr2;
      if (max1 > min1)
	{
	os << indent1 << vtkImageAxisNameMacro(temp[1]) 
	   << " range:(" << min1 << ", " << max1 << "), coordinant: (0, 0, "
	   << idx2 << ", " << idx3 << ")---------------------------\n";
	}
      for (idx1 = min1; idx1 <= max1; ++idx1)
	{

	indent0 = indent1.GetNextIndent();
	ptr0 = ptr1;
	os << indent0 << vtkImageAxisNameMacro(temp[0]) << ": " 
	   << (float)(*ptr0);
	ptr0 += inc0;
	for (idx0 = min0+1; idx0 <= max0; ++idx0)
	  {
	  os << ", " << (float)(*ptr0);
	  ptr0 += inc0;
	  }
	os << "\n";
	
	ptr1 += inc1;
	}

      ptr2 += inc2;
      }

    ptr3 += inc3;
    }
  
  // Set the precision value back to its original value.
  os.precision(precisionSave);
}




//----------------------------------------------------------------------------
void vtkImageData::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  vtkScalars *scalars;
  vtkIndent nextIndent = indent.GetNextIndent();
  
  vtkRefCount::PrintSelf(os,indent);
  os << indent << "Type: " << vtkImageScalarTypeNameMacro(this->ScalarType) 
     << "\n";
  
  os << indent << "Axes: (" << vtkImageAxisNameMacro(this->Axes[0]);
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << vtkImageAxisNameMacro(this->Axes[idx]);
    }
  os << ")\n";
  
  os << indent << "Extent: (" << this->Extent[0];
  for (idx = 1; idx < VTK_IMAGE_EXTENT_DIMENSIONS; ++idx)
    {
    os << ", " << this->Extent[idx];
    }
  os << ")\n";

  os << indent << "Increments: (" << this->Increments[0];
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << this->Increments[idx];
    }
  os << ")\n";

  os << indent << "PointData:\n";
  this->PointData.PrintSelf(os,nextIndent);
  // Adding this onto pointData (but in this class).
  if (this->PrintScalars)
    {
    scalars = this->PointData.GetScalars();
    if (scalars) 
      {
      void *ptr = this->GetScalarPointer();
      os << indent << "Scalar Values:\n";
      switch (this->GetScalarType())
	{
	case VTK_FLOAT:
	  vtkImageDataPrintScalars(this, (float *)(ptr), os, nextIndent);
	  break;
	case VTK_INT:
	  vtkImageDataPrintScalars(this, (int *)(ptr), os, nextIndent);
	  break;
	case VTK_SHORT:
	  vtkImageDataPrintScalars(this, (short *)(ptr), os, nextIndent);
	  break;
	case VTK_UNSIGNED_SHORT:
	  vtkImageDataPrintScalars(this, (unsigned short *)(ptr), 
				   os, nextIndent);
	  break;
	case VTK_UNSIGNED_CHAR:
	  vtkImageDataPrintScalars(this, (unsigned char *)(ptr),
				   os, nextIndent);
	  break;
	default:
	  os << nextIndent << "Cannot handle ScalarType.\n";
	}         
      }
    }
  
}


//----------------------------------------------------------------------------
// Description:
// This method tells the data object to handle a specific ScalarType.
// The method should be called before the data object is allocated.
void vtkImageData::SetScalarType(int type)
{
  if (this->GetRefCount() > 1)
    {
    vtkWarningMacro(<< "SetScalarType: " 
    << "This object has more than one reference!");
    }
  
  if (this->ScalarsAllocated)
    {
    vtkErrorMacro(<< "SetScalarType: Data object has already been allocated.");
    return;
    }
  
  this->Modified();
  this->ScalarType = type;
}




//----------------------------------------------------------------------------
// This is the same as vtkImageRegion::SetAxes(int num, int *axes).  Should
// we make a comman supperclass?
void vtkImageData::SetAxes(int num, int *axes)
{
  int idx, unusedAxis;
  int usedAxes[VTK_IMAGE_DIMENSIONS];
    
  // Error checking
  if (this->GetRefCount() > 1)
    {
    vtkWarningMacro(<< "SetAxes: This object has more than one reference!");
    }
  if (this->ScalarsAllocated)
    {
    vtkErrorMacro(<< "SetAxes: Data object has already been allocated.");
    return;
    }
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro("SetAxes: Too many axes specified");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  
  this->Modified();
  // The usedAxes array will be used to determine of if an axis repeates,
  // and to set the unspecified axes. There is an easier way to do this.
  // swap axeses as they are being set.  However, the unspecified axes
  // will not be sorted.
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    usedAxes[idx] = 0;
    // Because we cannot shuffle extent/increments in vtkImageData yet.
    this->Extent[idx*2] = this->Extent[idx*2+1] = 0;
    this->Increments[idx] = 0;
    }
  // Copy the specified axes.
  for (idx = 0; idx < num; ++idx)
    {
    // Check range
    if (axes[idx] < 0 || axes[idx] >= VTK_IMAGE_DIMENSIONS)
      {
      vtkErrorMacro("SetAxes: Axis " << axes[idx] << " is out of range");
      return;
      }
    // Check for repeated axes
    if (usedAxes[axes[idx]])
      {
      vtkErrorMacro("SetAxes: Axis " << axes[idx] << " is repeated");
      return;
      }
    // Set this specified axis
    usedAxes[axes[idx]] = 1;
    this->Axes[idx] = axes[idx];
    }
  // Set the unspecified axes.
  unusedAxis = 0;
  for (idx = num; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    // find the next unused axis.
    while (usedAxes[unusedAxis] && unusedAxis < VTK_IMAGE_DIMENSIONS)
      {
      ++unusedAxis;
      }
    // sanity check
    if (unusedAxis == VTK_IMAGE_DIMENSIONS)
      {
      vtkErrorMacro("SetAxis: Could not find an unused axis for " << idx);
      return;
      }
    // Set this unspecified axis
    usedAxes[unusedAxis] = 1;
    this->Axes[idx] = unusedAxis;
    }
}

//----------------------------------------------------------------------------
void vtkImageData::GetAxes(int num, int *axes)
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
void vtkImageData::SetExtent(int dim, int *extent)
{
  int idx;
  vtkDebugMacro(<< "SetExtent: ...");

  this->Modified();
  if (this->GetRefCount() > 1)
    {
    vtkWarningMacro(<< "SetExtent: This object has more than one reference!");
    }
  if (this->ScalarsAllocated)
    {
    vtkErrorMacro(<< "SetExtent: Data object has already been allocated.");
    return;
    }
  
  // Copy the input
  for (idx = 0; idx < dim; ++idx)
    {
    this->Extent[idx*2] = extent[idx*2];
    this->Extent[idx*2 + 1] = extent[idx*2 + 1];
    }
  
  // Set the unspecified axes.
  for (idx = dim; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Extent[idx*2] = this->Extent[idx*2 + 1] = 0;
    }
  
  // set up increments and volumes
  this->ComputeIncrements();
}


//----------------------------------------------------------------------------
void vtkImageData::GetExtent(int dim, int *extent)
{
  int idx;

  for (idx = 0; idx < 2*dim; ++idx)
    {
    extent[idx] = this->Extent[idx];
    }
}


//----------------------------------------------------------------------------
// Description:
// This method computes the increments and also computes the "Volume".
void vtkImageData::ComputeIncrements()
{
  int idx;
  int inc = 1;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Increments[idx] = inc;
    inc *= (this->Extent[idx*2+1] - this->Extent[idx*2] + 1);
    }
  
  this->Volume = inc;
}


//----------------------------------------------------------------------------
void vtkImageData::GetIncrements(int dim, int *increments)
{
  int idx;
  
  for (idx = 0; idx < dim; ++idx)
    {
    increments[idx] = this->Increments[idx];
    }
}


//----------------------------------------------------------------------------
// Description:
// A convenience function that will translate the extent ot the data object,
// without changing its dimensions.  It can be called after the data has been
// allocated.
void vtkImageData::Translate(int vector[VTK_IMAGE_DIMENSIONS])
{
  int idx;
  
  if (this->GetRefCount() > 1)
    {
    vtkWarningMacro(<< "Translate: This object has more than one reference!");
    }
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Extent[idx*2] += vector[idx];
    this->Extent[1+idx*2] += vector[idx];
    }
}

//----------------------------------------------------------------------------
// Description:
// This method returns 1 if the scalar data has been allocated.
int vtkImageData::AreScalarsAllocated()
{
  if (this->PointData.GetScalars())
    return 1;
  else
    return 0;
}



//----------------------------------------------------------------------------
// Description:
// This method allocates memory for the vtkImageData scalars.  
// The extent of the data object and the ScalarType should be 
// set before this method is called.
// The method returns 1 if the allocation was sucessful, 0 if not.
int vtkImageData::AllocateScalars()
{
  vtkScalars* scalars = NULL;
  

  // special case zero length array
  if (this->Volume <= 0)
    {
    this->PointData.SetScalars(NULL);
    return 1;
    }
  
  // create the PointData object.
  switch (this->ScalarType)
    {
    case VTK_VOID:
      vtkErrorMacro(<< "AllocateScalars: ScalarType Unknown");
      break;
    case VTK_FLOAT:
      scalars = vtkFloatScalars::New();
      this->ScalarsAllocated = scalars->Allocate(this->Volume);
      ((vtkFloatScalars *)(scalars))->WritePtr(0,this->Volume);
      break;
    case VTK_INT:
      scalars = vtkIntScalars::New();
      this->ScalarsAllocated = scalars->Allocate(this->Volume);
      ((vtkIntScalars *)(scalars))->WritePtr(0,this->Volume);
      break;
    case VTK_SHORT:
      scalars = vtkShortScalars::New();
      this->ScalarsAllocated =  scalars->Allocate(this->Volume);
      ((vtkShortScalars *)(scalars))->WritePtr(0,this->Volume);
      break;
    case VTK_UNSIGNED_SHORT:
      scalars = vtkUnsignedShortScalars::New();
      this->ScalarsAllocated = scalars->Allocate(this->Volume);
      ((vtkUnsignedShortScalars *)(scalars))->WritePtr(0,this->Volume);
      break;
    case VTK_UNSIGNED_CHAR:
      scalars = vtkUnsignedCharScalars::New();
      this->ScalarsAllocated = scalars->Allocate(this->Volume);
      ((vtkUnsignedCharScalars *)(scalars))->WritePtr(0,this->Volume);
      break;
    }
  
  if (this->ScalarsAllocated)
    {
    this->PointData.SetScalars(scalars);
    }
  // Delete scalars, since PointData refernece counts scalars.
  scalars->Delete();

  return this->ScalarsAllocated;
}




//----------------------------------------------------------------------------
// Description:
// This method makes sure the scalars are allocated, 
// and we have the only reference.  It copies the scalars if necessary.
void vtkImageData::MakeScalarsWritable()
{
  vtkScalars* scalars = this->PointData.GetScalars();
  
  // special case zero length array
  if (this->Volume <= 0)
    {
    return;
    }
  
  // make sure the scalars are allocated
  if ( ! scalars)
    {
    this->AllocateScalars(); 
    scalars = this->PointData.GetScalars();
    }
  
  // We should also make sure there are enough scalars for volume.
  // ... switch ...
  
  // Make sure we have the only reference to the scalars.
  if (scalars->GetRefCount() > 1)
    {
    vtkScalars *newScalars;
    // Scalars need to be copied (some one else is referencing them)
    switch (this->GetScalarType())
      {
      case VTK_FLOAT:
	newScalars = new vtkFloatScalars();
	*((vtkFloatScalars *)newScalars) = *((vtkFloatScalars *)scalars);
	break;
      case VTK_INT:
	newScalars = new vtkIntScalars();
	*((vtkIntScalars *)newScalars) = *((vtkIntScalars *)scalars);
	break;
      case VTK_SHORT:
	newScalars = new vtkShortScalars();
	*((vtkShortScalars *)newScalars) = *((vtkShortScalars *)scalars);
	break;
      case VTK_UNSIGNED_SHORT:
	newScalars = new vtkUnsignedShortScalars();
	*((vtkUnsignedShortScalars *)newScalars) 
	  = *((vtkUnsignedShortScalars *)scalars);
	break;
      case VTK_UNSIGNED_CHAR:
	newScalars = new vtkUnsignedCharScalars();
	*((vtkUnsignedCharScalars *)newScalars) 
	  = *((vtkUnsignedCharScalars *)scalars);
	break;
      default:
	vtkErrorMacro(<< "MakeScalarsWritable: Cannot handle ScalarType.\n");
	return;
      }

    // Automatically dereferences old scalars and references new scalars.
    this->PointData.SetScalars(newScalars);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Description:
// You can set the scalars directly (instead of allocating), but
// you better make sure that the extent are set properly 
// before this method is called. 
// This method is here (instead of GetPointData()->SetScalars)
// because ScalarType Needs to be set.  This may change in the future.
void vtkImageData::SetScalars(vtkScalars *scalars)
{
  int num;

  if (! scalars)
    {
    this->PointData.SetScalars(scalars);
    this->ScalarType = VTK_VOID;
    return;
    }
  
  // Set the proper type.
  if (strcmp(scalars->GetDataType(), "float") == 0)
    {
    this->ScalarType = VTK_FLOAT;
    num = ((vtkFloatScalars *)(scalars))->GetNumberOfScalars();
    }
  else if (strcmp(scalars->GetDataType(), "int") == 0)
    {
    this->ScalarType = VTK_INT;
    num = ((vtkIntScalars *)(scalars))->GetNumberOfScalars();
    }
  else if (strcmp(scalars->GetDataType(), "short") == 0)
    {
    this->ScalarType = VTK_SHORT;
    num = ((vtkShortScalars *)(scalars))->GetNumberOfScalars();
    }
  else if (strcmp(scalars->GetDataType(), "unsigned short") == 0)
    {
    this->ScalarType = VTK_UNSIGNED_SHORT;
    num = ((vtkUnsignedShortScalars *)(scalars))->GetNumberOfScalars();
    }
  else if (strcmp(scalars->GetDataType(), "unsigned char") == 0)
    {
    this->ScalarType = VTK_UNSIGNED_CHAR;
    num = ((vtkUnsignedCharScalars *)(scalars))->GetNumberOfScalars();
    }
  else
    {
    vtkErrorMacro(<< "SetScalars: Cannot handle "<< scalars->GetClassName());
    return;
    }

  // Check to see if the number of scalar matches volume.
  if (num != this->Volume)
    {
    vtkErrorMacro("SetScalars: The number of scalars " << num 
		  << " does not match the volume " << this->Volume);
    }
  
  this->PointData.SetScalars(scalars);
  this->Modified();
}


//----------------------------------------------------------------------------
// Description:
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetScalarPointer(int coordinates[VTK_IMAGE_DIMENSIONS])
{
  vtkScalars *scalars;
  int idx;
    
  // Make sure the scalars have been allocated.
  scalars = this->PointData.GetScalars();
  if (scalars == NULL)
    {
    this->AllocateScalars();
    scalars = this->PointData.GetScalars();    
    if (scalars == NULL)
      {
      vtkErrorMacro("Can't allocate scalars");
      // for debugging
      this->AllocateScalars();
      scalars = this->PointData.GetScalars();    
      }
    }
  
  // error checking: since most acceses will be from pointer arithmetic.
  // this should not waste much time.
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (coordinates[idx] < this->Extent[idx*2] ||
	coordinates[idx] > this->Extent[idx*2+1])
      {
      vtkErrorMacro(<< "GetScalarPointer: Pixel (" << coordinates[0] << ", " 
                    << coordinates[1] << ", " << coordinates[2] << ", "
                    << coordinates[3] << ") not in memory.");
      return NULL;
      }
    }
  
  // compute the index of the vector.
  idx = ((coordinates[0] - this->Extent[0]) * this->Increments[0]
	 + (coordinates[1] - this->Extent[2]) * this->Increments[1]
	 + (coordinates[2] - this->Extent[4]) * this->Increments[2]
	 + (coordinates[3] - this->Extent[6]) * this->Increments[3]
	 + (coordinates[4] - this->Extent[8]) * this->Increments[4]);
  
  return scalars->GetVoidPtr(idx);
}


//----------------------------------------------------------------------------
// Description:
// This Method returns a pointer to the origin of the vtkImageData.
void *vtkImageData::GetScalarPointer()
{
  if (this->PointData.GetScalars() == NULL)
    {
    this->AllocateScalars();
    }
  return this->PointData.GetScalars()->GetVoidPtr(0);
}



