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
#include "vtkImageRegion.h"

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
  this->MemoryOrder[0] = VTK_IMAGE_COMPONENT_AXIS;
  this->MemoryOrder[1] = VTK_IMAGE_X_AXIS;
  this->MemoryOrder[2] = VTK_IMAGE_Y_AXIS;
  this->MemoryOrder[3] = VTK_IMAGE_Z_AXIS;
  this->MemoryOrder[4] = VTK_IMAGE_TIME_AXIS;
  this->NumberOfScalars = 1;
}


//----------------------------------------------------------------------------
// A templated function to print a single pixel.
template <class T>
static void vtkImageDataPrintPixel(vtkImageData *self, T *ptr, ostream& os)
{
  int idx;
  int inc;
  int min, max;

  self->GetAxisIncrement(VTK_IMAGE_COMPONENT_AXIS, inc);
  self->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min, max);

  if (min == max)
    { // one component, just print it.
    os << *ptr;
    return;
    }
  
  os << "(" << *ptr;
  for (idx = min+1; idx <= max; ++idx)
    {
    ptr += inc;
    os << ", " << *ptr;
    }
  os << ")";
  
  return;
}


//----------------------------------------------------------------------------
// A method that prints all the data.
// If an axis has no data (colapsed: min == max), it is not printed.
template <class T>
static void vtkImageDataPrintScalars(vtkImageData *self, T *ptr,
				     ostream& os, vtkIndent indent)
{
  int precisionSave = os.precision();
  int idxX, idxY, idxZ, idxT;
  int incX, incY, incZ, incT;
  int minX, maxX, minY, maxY, minZ, maxZ, minT, maxT;
  T *ptrX, *ptrY, *ptrZ, *ptrT;
  vtkIndent indentY, indentZ, indentT;
  int numY, numZ, numT;
  int numberOfAxes = 0;

  // Only print float values to 2 decimals
  os.precision(2);
  
  self->GetAxisIncrement(VTK_IMAGE_X_AXIS, incX);
  self->GetAxisIncrement(VTK_IMAGE_Y_AXIS, incY);
  self->GetAxisIncrement(VTK_IMAGE_Z_AXIS, incZ);
  self->GetAxisIncrement(VTK_IMAGE_TIME_AXIS, incT);

  self->GetAxisExtent(VTK_IMAGE_X_AXIS, minX, maxX);
  if (maxX > minX) ++numberOfAxes;
  self->GetAxisExtent(VTK_IMAGE_Y_AXIS, minY, maxY);
  if (maxY > minY) ++numberOfAxes;
  numY = numberOfAxes;
  self->GetAxisExtent(VTK_IMAGE_Z_AXIS, minZ, maxZ);
  if (maxZ > minZ) ++numberOfAxes;
  numZ = numberOfAxes;
  self->GetAxisExtent(VTK_IMAGE_TIME_AXIS, minT, maxT);
  if (maxT > minT) ++numberOfAxes;
  numT = numberOfAxes;
  
  indentT = indent;
  ptrT = ptr;
  for (idxT = minT; idxT <= maxT; ++idxT)
    {
    ptrZ = ptrT;
    indentZ = indentT;
    if (maxT > minT)
      {
      if (numT > 1)
	{
	os << indentT << "T(" << idxT << "): ";
	}
      if (numT > 2)
	{
	os << "\n";
	}
      indentZ = indentT.GetNextIndent();
      }
    for (idxZ = minZ; idxZ <= maxZ; ++idxZ)
      {
      ptrY = ptrZ;
      indentY = indentZ;
      if (maxZ > minZ)
	{
	if (numZ > 1)
	  {
	  os << indentZ << "Z(" << idxZ << "): ";
	  }
	if (numZ > 2)
	  {
	  os << "\n";
	  }
	indentY = indentZ.GetNextIndent();
	}
      for (idxY = minY; idxY <= maxY; ++idxY)
	{
	ptrX = ptrY;
	if (maxY > minY)
	  {
	  if (numY > 1)
	    {
	    os << indentY << "Y(" << idxY << "): ";
	    }
	  }
	for (idxX = minX; idxX <= maxX; ++idxX)
	  {
	  vtkImageDataPrintPixel(self, ptrX, os);
	  os << " ";
	  ptrX += incX;
	  }
	ptrY += incY;
	}
      ptrZ += incZ;
      }
    ptrT += incT;
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
  
  vtkReferenceCount::PrintSelf(os,indent);
  os << indent << "Type: " << vtkImageScalarTypeNameMacro(this->ScalarType) 
     << "\n";
  
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
void vtkImageData::SetExtent(int num, int *extent)
{
  int idx, modified = 0;
  vtkDebugMacro(<< "SetExtent: ...");

  if (this->ScalarsAllocated)
    {
    vtkErrorMacro(<< "SetExtent: Data object has already been allocated.");
    return;
    }

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro("SetExtent: num = " << num << " is too large.");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  // Copy the input
  for (idx = 0; idx < num; ++idx)
    {
    if (this->Extent[idx*2] != extent[idx*2])
      {
      this->Extent[idx*2] = extent[idx*2];
      modified = 1;
      }
    if (this->Extent[idx*2 + 1] != extent[idx*2 + 1])
      {
      this->Extent[idx*2 + 1] = extent[idx*2 + 1];
      modified = 1;
      }
    }

  if (modified)
    {
    this->Modified();
    this->ComputeIncrements();
    }
}

//----------------------------------------------------------------------------
void vtkImageData::SetAxisExtent(int axis, int min, int max)
{
  int modified = 0;
  
  if (axis < 0 || axis > 4)
    {
    vtkErrorMacro("SetAxisExtent: Bad axis " << axis);
    return;
    }
  if (this->Extent[axis*2] != min)
    {
    this->Extent[axis*2] = min;
    modified = 1;
    }
  if (this->Extent[axis*2+1] != max)
    {
    this->Extent[axis*2+1] = max;
    modified = 1;
    }
  if (modified)
    {
    this->Modified();
    this->ComputeIncrements();
    }
}

//----------------------------------------------------------------------------
void vtkImageData::GetAxisExtent(int axis, int &min, int &max)
{
  if (axis < 0 || axis > 4)
    {
    vtkErrorMacro("GetAxisExtent: Bad axis " << axis);
    min = max = 0;
    return;
    }
  min = this->Extent[axis*2];
  max = this->Extent[axis*2+1];
}

//----------------------------------------------------------------------------
void vtkImageData::GetAxes(int num, int *axes)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro("GetAxes: Too many axes requested " << num);
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    axes[idx] = this->Axes[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageData::GetMemoryOrder(int num, int *axes)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro("GetMemoryOrder: Too many axes requested " << num);
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    axes[idx] = this->MemoryOrder[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageData::SetScalarType(int type)
{
  if (type == this->ScalarType)
    {
    return;
    }
  
  if (this->GetReferenceCount() > 1)
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
void vtkImageData::GetIncrements(int dim, int *increments)
{
  int idx;
  
  for (idx = 0; idx < dim; ++idx)
    {
    increments[idx] = this->Increments[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageData::GetAxisIncrement(int axis, int &inc)
{
  if (axis < 0 || axis > 4)
    {
    vtkErrorMacro("GetAxisIncrement: Bad axis " << axis);
    inc = 0;
    return;
    }
  inc = this->Increments[axis];
}

//----------------------------------------------------------------------------
// Description:
// This method computes the increments from the MemoryOrder and the extent.
// It also computes "NumberOfScalars".
void vtkImageData::ComputeIncrements()
{
  int idx, axis;
  int inc = 1;

  // initialize for test that memory order is 1 to 1.
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Increments[idx] = 0;
    }
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    axis = this->MemoryOrder[idx];
    if (axis < 0 || axis > 4)
      {
      vtkErrorMacro("ComputeIncrements: Bad MemoryOrder[" << idx 
		    << "] axis " << axis);
      return;
      }
    if (this->Increments[axis])
      { // Axis repeated
      vtkErrorMacro("ComputeIncrements: MemoryOrder[" << idx 
		    << "] repeated axis " << axis);
      return;
      }
    this->Increments[axis] = inc;
    inc *= (this->Extent[axis*2+1] - this->Extent[axis*2] + 1);
    }
  this->NumberOfScalars = inc;
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
  if (this->NumberOfScalars <= 0)
    {
    this->PointData.SetScalars(NULL);
    return 1;
    }
  
  // create the PointData object.
  switch (this->ScalarType)
    {
    case VTK_VOID:
      vtkErrorMacro(<< "AllocateScalars: ScalarType Unknown");
      return 0;
    case VTK_FLOAT:
      scalars = vtkFloatScalars::New();
      this->ScalarsAllocated = scalars->Allocate(this->NumberOfScalars);
      ((vtkFloatScalars *)(scalars))->WritePointer(0,this->NumberOfScalars);
      break;
    case VTK_INT:
      scalars = vtkIntScalars::New();
      this->ScalarsAllocated = scalars->Allocate(this->NumberOfScalars);
      ((vtkIntScalars *)(scalars))->WritePointer(0,this->NumberOfScalars);
      break;
    case VTK_SHORT:
      scalars = vtkShortScalars::New();
      this->ScalarsAllocated =  scalars->Allocate(this->NumberOfScalars);
      ((vtkShortScalars *)(scalars))->WritePointer(0,this->NumberOfScalars);
      break;
    case VTK_UNSIGNED_SHORT:
      scalars = vtkUnsignedShortScalars::New();
      this->ScalarsAllocated = scalars->Allocate(this->NumberOfScalars);
      ((vtkUnsignedShortScalars *)
       (scalars))->WritePointer(0,this->NumberOfScalars);
      break;
    case VTK_UNSIGNED_CHAR:
      scalars = vtkUnsignedCharScalars::New();
      this->ScalarsAllocated = scalars->Allocate(this->NumberOfScalars);
      ((vtkUnsignedCharScalars *)
       (scalars))->WritePointer(0,this->NumberOfScalars);
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
  if (this->NumberOfScalars <= 0)
    {
    return;
    }
  
  // make sure the scalars are allocated
  if ( ! scalars)
    {
    this->AllocateScalars(); 
    scalars = this->PointData.GetScalars();
    }
  
  // We should also make sure there are enough scalars for NumberOfScalars.
  // ... switch ...
  
  // Make sure we have the only reference to the scalars.
  if (scalars->GetReferenceCount() > 1)
    {
    vtkScalars *newScalars;
    // Scalars need to be copied (some one else is referencing them)
    switch (this->GetScalarType())
      {
      case VTK_FLOAT:
	newScalars = new vtkFloatScalars;
	*((vtkFloatScalars *)newScalars) = *((vtkFloatScalars *)scalars);
	break;
      case VTK_INT:
	newScalars = new vtkIntScalars;
	*((vtkIntScalars *)newScalars) = *((vtkIntScalars *)scalars);
	break;
      case VTK_SHORT:
	newScalars = new vtkShortScalars;
	*((vtkShortScalars *)newScalars) = *((vtkShortScalars *)scalars);
	break;
      case VTK_UNSIGNED_SHORT:
	newScalars = new vtkUnsignedShortScalars;
	*((vtkUnsignedShortScalars *)newScalars) 
	  = *((vtkUnsignedShortScalars *)scalars);
	break;
      case VTK_UNSIGNED_CHAR:
	newScalars = new vtkUnsignedCharScalars;
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

  // Check to see if the number of scalar matches NumberOfScalars.
  if (num != this->NumberOfScalars)
    {
    vtkErrorMacro("SetScalars: The number of scalars " << num 
		  << " does not match the NumberOfScalars " 
		  << this->NumberOfScalars);
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









