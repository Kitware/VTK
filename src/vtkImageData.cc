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

  this->Data = NULL;
  this->Length = 0;
  for (idx = 0; idx < 3; ++idx)
    this->Size[idx] = this->Offset[idx] = this->Inc[idx] = 0;
}

//----------------------------------------------------------------------------
vtkImageData::~vtkImageData()
{
  if (this->Data)
    delete [] this->Data;
}


//----------------------------------------------------------------------------
// Description:
// This method returns 1 if the data object has already been allocated.
int vtkImageData::Allocated()
{
  if (this->Data)
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
  if (this->Data)
    {
    delete [] this->Data;
    this->Data = NULL;
    this->Length = 0;
    }
  
  // set up size and increments
  for (idx = 0; idx < 3; ++idx)
    {
    this->Inc[idx] = inc;
    inc *= this->Size[idx];
    }
  
  // allocate more memory
  this->Data = new float[inc];
  if (this->Data)
    {
    this->Length = inc;
    return 1;
    } 
  else 
    {
    // allocation of data failed.
    vtkErrorMacro(<< "Allocate: Could not allocate memory.");
    return 0;
    }
}


//----------------------------------------------------------------------------
// Description:
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
float *vtkImageData::GetPointer(int coordinates[3])
{
  return this->Data 
    + (coordinates[0] - this->Offset[0]) * this->Inc[0]
      + (coordinates[1] - this->Offset[1]) * this->Inc[1]
	+ (coordinates[2] - this->Offset[2]) * this->Inc[2];
}














