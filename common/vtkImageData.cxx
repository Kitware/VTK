/*=========================================================================


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
#include "vtkFloatScalars.h"
#include "vtkIntScalars.h"
#include "vtkShortScalars.h"
#include "vtkUnsignedShortScalars.h"
#include "vtkGraymap.h"
#include "vtkPixmap.h"
#include "vtkAGraymap.h"
#include "vtkAPixmap.h"

//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageData with no data.
vtkImageData::vtkImageData()
{
  int idx;

  for (idx = 0; idx < 3; ++idx)
    {
    this->Increments[idx] = 0;
    this->Extent[idx*2] = 0;
    this->Extent[idx*2 + 1] = 0;
    }
  
  this->ScalarType = VTK_VOID;
  this->NumberOfScalarComponents = 0; 
}


//----------------------------------------------------------------------------
void vtkImageData::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkStructuredPoints::PrintSelf(os,indent);

  os << indent << "Extent: (" << this->Extent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->Extent[idx];
    }
  os << ")\n";

  os << indent << "Increments: (" << this->Increments[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->Increments[idx];
    }
  os << ")\n";

  os << indent << "NumberOfScalarComponents: " << 
    this->NumberOfScalarComponents << endl;
  os << indent << "ScalarType: " << this->ScalarType << endl;
  
}


//----------------------------------------------------------------------------
void vtkImageData::SetNumberOfScalarComponents(int num)
{
  int modified = 0;
  
  if (this->NumberOfScalarComponents != num)
    {
    this->NumberOfScalarComponents = num;
    modified = 1;
    }
  
  if (modified)
    {
    this->Modified();
    this->ComputeIncrements();
    }
}

void vtkImageData::GetExtent(int &x1, int &x2, int &y1, int &y2, 
			     int &z1, int &z2)
{
  int *ext;
  ext = this->GetExtent();
  x1 = ext[0];
  x2 = ext[1];
  y1 = ext[2];
  y2 = ext[3];
  z1 = ext[4];
  z2 = ext[5];
}

//----------------------------------------------------------------------------
void vtkImageData::SetExtent(int *extent)
{
  int idx, modified = 0;
  vtkDebugMacro(<< "SetExtent: ...");
  
  // Copy the input
  for (idx = 0; idx < 3; ++idx)
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
void vtkImageData::GetContinuousIncrements(int extent[6], int &incX,
					   int &incY, int &incZ)
{
  int e0, e1, e2, e3;
  
  incX = 0;

  e0 = extent[0];
  if (e0 < this->Extent[0]) e0 = this->Extent[0];
  e1 = extent[1];
  if (e1 > this->Extent[1]) e1 = this->Extent[1];
  e2 = extent[2];
  if (e2 < this->Extent[2]) e2 = this->Extent[2];
  e3 = extent[3];
  if (e3 > this->Extent[3]) e3 = this->Extent[3];
  
  incY = this->Increments[1] - (e1 - e0 + 1)*this->Increments[0];
  incZ = this->Increments[2] - (e3 - e2 + 1)*this->Increments[1];
}


//----------------------------------------------------------------------------
// Description:
// This method computes the increments from the MemoryOrder and the extent.
void vtkImageData::ComputeIncrements()
{
  int idx;
  int inc = this->NumberOfScalarComponents;

  for (idx = 0; idx < 3; ++idx)
    {
    this->Increments[idx] = inc;
    inc *= (this->Extent[idx*2+1] - this->Extent[idx*2] + 1);
    }
}


//----------------------------------------------------------------------------
// Description:
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetScalarPointer(int x, int y, int z)
{
  int tmp[3];
  tmp[0] = x;
  tmp[1] = y;
  tmp[2] = z;
  return this->GetScalarPointer(tmp);
}

//----------------------------------------------------------------------------
// Description:
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetScalarPointerForExtent(int extent[6])
{
  int tmp[3];
  tmp[0] = extent[0];
  tmp[1] = extent[2];
  tmp[2] = extent[4];
  return this->GetScalarPointer(tmp);
}

//----------------------------------------------------------------------------
// Description:
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetScalarPointer(int coordinates[3])
{
  vtkScalars *scalars;
  int idx;
    
  // Make sure the scalars have been allocated.
  scalars = this->PointData.GetScalars();
  if (scalars == NULL)
    {
    vtkDebugMacro("Allocating scalars in ImageData");
    this->AllocateScalars();
    scalars = this->PointData.GetScalars();
    }
  
  // error checking: since most acceses will be from pointer arithmetic.
  // this should not waste much time.
  for (idx = 0; idx < 3; ++idx)
    {
    if (coordinates[idx] < this->Extent[idx*2] ||
	coordinates[idx] > this->Extent[idx*2+1])
      {
      vtkErrorMacro(<< "GetScalarPointer: Pixel (" << coordinates[0] << ", " 
      << coordinates[1] << ", "
      << coordinates[2] << ") not in memory.\n Current extent= ("
      << this->Extent[0] << ", " << this->Extent[1] << ", "
      << this->Extent[2] << ", " << this->Extent[3] << ", "
      << this->Extent[4] << ", " << this->Extent[5] << ")");
      return NULL;
      }
    }
  
  // compute the index of the vector.
  idx = ((coordinates[0] - this->Extent[0]) * this->Increments[0]
	 + (coordinates[1] - this->Extent[2]) * this->Increments[1]
	 + (coordinates[2] - this->Extent[4]) * this->Increments[2]);
  
  return scalars->GetVoidPtr(idx);
}


//----------------------------------------------------------------------------
// Description:
// This Method returns a pointer to the origin of the vtkImageData.
void *vtkImageData::GetScalarPointer()
{
  if (this->PointData.GetScalars() == NULL)
    {
    vtkDebugMacro("Allocating scalars in ImageData");
    this->AllocateScalars();
    }
  return this->PointData.GetScalars()->GetVoidPtr(0);
}

static int vtkGetScalarType(vtkImageData *self)
{
  vtkScalars *tmp;
  
  tmp = self->GetPointData()->GetScalars();
  if (!tmp) return VTK_VOID;
  
  return tmp->GetDataType();
}

//----------------------------------------------------------------------------
void vtkImageData::AllocateScalars()
{
  // if the scalar type has not been set then we have a problem
  if (this->ScalarType == VTK_VOID)
    {
    vtkWarningMacro("Attempt to allcoate scalars before scalar type was set!.");
    return;
    }
  
  // if the current type matches the new type then ignore
  if (vtkGetScalarType(this) == this->ScalarType) 
    {
    this->PointData.GetScalars()->
      SetNumberOfScalars((this->Extent[1] - this->Extent[0] + 1)*
			 (this->Extent[3] - this->Extent[2] + 1)*
			 (this->Extent[5] - this->Extent[4] + 1));
    return;
    }
  
  // otherwise delete the old data (if any) 
  if (this->PointData.GetScalars())
    {
    this->PointData.SetScalars(NULL);
    }
  
  // allocate the new scalars
  switch (this->ScalarType)
    {
    case VTK_FLOAT:
      this->PointData.SetScalars(vtkFloatScalars::New());
      break;
    case VTK_INT:
      this->PointData.SetScalars(vtkIntScalars::New());
      break;
    case VTK_SHORT:
      this->PointData.SetScalars(vtkShortScalars::New());
      break;
    case VTK_UNSIGNED_SHORT:
      this->PointData.SetScalars(vtkUnsignedShortScalars::New());
      break;
    case VTK_UNSIGNED_CHAR:
      // this depends on how many components we have
      switch (this->NumberOfScalarComponents)
	{
	case 1:
	  this->PointData.SetScalars(vtkGraymap::New());
	  break;
	case 2:
	  this->PointData.SetScalars(vtkAGraymap::New());
	  break;
	case 3:
	  this->PointData.SetScalars(vtkPixmap::New());
	  break;
	case 4:
	  this->PointData.SetScalars(vtkAPixmap::New());
	  break;
	}
      break;
    }
  
  // Unregister the scalars since PointData now has a references
  this->PointData.GetScalars()->UnRegister(this);
  
  // allocate enough memory
  this->PointData.GetScalars()->
    SetNumberOfScalars((this->Extent[1] - this->Extent[0] + 1)*
		       (this->Extent[3] - this->Extent[2] + 1)*
		       (this->Extent[5] - this->Extent[4] + 1));
}


int vtkImageData::GetScalarSize()
{
  // allocate the new scalars
  switch (this->ScalarType)
    {
    case VTK_FLOAT:
      return sizeof(float);
      break;
    case VTK_INT:
      return sizeof(int);
      break;
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
      return 2;
      break;
    case VTK_UNSIGNED_CHAR:
      return 1;
      break;
    }
  
  return 1;
}




