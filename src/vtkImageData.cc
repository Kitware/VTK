/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageData.cc
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
#include "vtkImageData.hh"

//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageData with no data.
vtkImageData::vtkImageData()
{
  int idx;

  this->Scalars = NULL;
  this->Allocated = 0;
  this->Type = VTK_IMAGE_VOID;
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Increments[idx] = 0;
    this->Bounds[idx*2] = 0;
    this->Bounds[idx*2 + 1] = 0;
    }
}

//----------------------------------------------------------------------------
vtkImageData::~vtkImageData()
{
  if (this->Scalars)
    {
    this->Scalars->Delete();
    this->Scalars = NULL;
    }
}




//----------------------------------------------------------------------------
// Description:
// This method sets the bounds of the data, and 
// should be called before the data object is allocated.
void vtkImageData::SetBounds(int min0, int max0, int min1, int max1, 
			     int min2, int max2, int min3, int max3,
			     int min4, int max4)
{
  vtkDebugMacro(<< "SetBounds: ...");

  if (this->Scalars)
    {
    vtkErrorMacro(<< "SetBounds: Data object has already been allocated.");
    return;
    }
  
  this->Modified();
  this->Bounds[0] = min0;
  this->Bounds[1] = max0;
  this->Bounds[2] = min1;
  this->Bounds[3] = max1;
  this->Bounds[4] = min2;
  this->Bounds[5] = max2;
  this->Bounds[6] = min3;
  this->Bounds[7] = max3;
  this->Bounds[8] = min4;
  this->Bounds[9] = max4;
}






//----------------------------------------------------------------------------
// Description:
// This method tells the data object to handle a specific DataType.
// The method should be called before the data object is allocated.
void vtkImageData::SetType(int type)
{
  if (this->Scalars)
    {
    vtkErrorMacro(<< "SetType: Data object has already been allocated.");
    return;
    }
  
  this->Modified();
  this->Type = type;
}



//----------------------------------------------------------------------------
// Description:
// This method returns 1 if the data object has already been allocated.
int vtkImageData::IsAllocated()
{
  if (this->Scalars)
    return 1;
  else
    return 0;
}



//----------------------------------------------------------------------------
// Description:
// This method allocates memory for the vtkImageData data.  The size of
// the data object should be set before this method is called.
// The method returns 1 if the allocation was sucessful, 0 if not.
int vtkImageData::Allocate()
{
  int idx, inc = 1;

  // delete previous data
  // in the future try to reuse memory
  if (this->Scalars)
    {
    this->Scalars->Delete();
    this->Scalars = NULL;
    }
  
  // set up increments
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Increments[idx] = inc;
    inc *= (this->Bounds[idx*2+1] - this->Bounds[idx*2] + 1);
    }
  
  // special case zero length array
  if (inc <= 0)
    {
    this->Scalars = NULL;
    return 1;
    }
  
  // create the Scalars object.
  switch (this->Type)
    {
    case VTK_IMAGE_VOID:
      vtkErrorMacro(<< "Allocate: Type Unknown");
      break;
    case VTK_IMAGE_FLOAT:
      this->Scalars = new vtkFloatScalars;
      this->Allocated = this->Scalars->Allocate(inc);
      ((vtkFloatScalars *)(this->Scalars))->WritePtr(0,inc);
      break;
    case VTK_IMAGE_INT:
      this->Scalars = new vtkIntScalars;
      this->Allocated = this->Scalars->Allocate(inc);
      ((vtkIntScalars *)(this->Scalars))->WritePtr(0,inc);
      break;
    case VTK_IMAGE_SHORT:
      this->Scalars = new vtkShortScalars;
      this->Allocated =  this->Scalars->Allocate(inc);
      ((vtkShortScalars *)(this->Scalars))->WritePtr(0,inc);
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      this->Scalars = new vtkUnsignedShortScalars;
      this->Allocated = this->Scalars->Allocate(inc);
      ((vtkUnsignedShortScalars *)(this->Scalars))->WritePtr(0,inc);
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      this->Scalars = new vtkUnsignedCharScalars;
      this->Allocated = this->Scalars->Allocate(inc);
      ((vtkUnsignedCharScalars *)(this->Scalars))->WritePtr(0,inc);
      break;
    }


  return this->Allocated;
}


//----------------------------------------------------------------------------
// Description:
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetVoidPointer(int coordinates[VTK_IMAGE_DIMENSIONS])
{
  int idx;
    
  // error checking: since most acceses will be from pointer arithmatic.
  // this should not waste much time.
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if (coordinates[idx] < this->Bounds[idx*2] ||
	coordinates[idx] > this->Bounds[idx*2+1])
      {
      vtkErrorMacro(<< "GetVoidPointer: Pixel (" << coordinates[0] << ", " 
                    << coordinates[1] << ", " << coordinates[2] << ", "
                    << coordinates[3] << ") not in memory.");
      return NULL;
      }
    }
  
  // Note the VTK data model (Scalars) does not exactly fit with
  // Image data model. We need a switch to get a void pointer.
  idx = ((coordinates[0] - this->Bounds[0]) * this->Increments[0]
	 + (coordinates[1] - this->Bounds[2]) * this->Increments[1]
	 + (coordinates[2] - this->Bounds[4]) * this->Increments[2]
	 + (coordinates[3] - this->Bounds[6]) * this->Increments[3]
	 + (coordinates[4] - this->Bounds[8]) * this->Increments[4]);
  
  return this->Scalars->GetVoidPtr(idx);
}


//----------------------------------------------------------------------------
// Description:
// This Method returns a pointer to the origin of the vtkImageData.
void *vtkImageData::GetVoidPointer()
{
  return this->Scalars->GetVoidPtr(0);
}














