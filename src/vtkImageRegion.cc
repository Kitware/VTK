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
  int idx;

  this->Data = NULL;
  for (idx = 0; idx < 3; ++idx)
    this->Size[idx] = this->Offset[idx] = 0;
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
// This method allocates memory for the region.  "Offset" and "Size"
// should be set before this method is called.  Any old "Data" associated 
// with this region is released by this call, and a new vtkImageData 
// object is created.
// The method returns 1 if it was successful, 0 if not.
int vtkImageRegion::Allocate()
{
  // delete previous data
  if (this->Data)
    {
    this->Data->Delete();
    this->Data = NULL;
    }

  // Create the data object
  this->Data = new vtkImageData;
  this->Data->SetSize(this->Size);
  this->Data->SetOffset(this->Offset);
  
  if ( ! this->Data->Allocate())
    {
    // Region is too big to fit into one data object 
    this->Data->Delete();
    this->Data = NULL;
    
    return 0;
    }    

  this->Modified();
  return 1;
}

//----------------------------------------------------------------------------
// Description:
// You can set the data object explicitly, instead of using the Allocate
// method.  Old data is released, and the region automatically registers
// the new data.
void vtkImageRegion::SetData(vtkImageData *data)
{
  // do nothing if the data objects are the same.
  if (this->Data == data)
    return;
  
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
}


//----------------------------------------------------------------------------
// Description:
// This Method returns a pointer at a location in the tile.  The coordinates
// of the location are in pixel units and are relative to the absolute
// origin of the whole image. The region just forwards the pointer request
// to its vtkImageData object.
float *vtkImageRegion::GetPointer(int coordinates[3])
{
  if ( ! this->Data){
    vtkErrorMacro(<<"Data must be set or allocated.");
    return NULL;
  }

  return this->Data->GetPointer(coordinates);
}



//----------------------------------------------------------------------------
// Description:
// This method returns the increments between pixels, rows, and images.
// These values are determined by the actual dimensions of the data stored
// in the vtkImageData object.  this method allows the user to efficiently 
// march through the memory using pointer arithmatic, while keeping the
// actual dimensions of the memory array transparent.  
// It also keeps the order (row order, image order ...) of the memory
// transparent, but this feature has not been used yet.
void vtkImageRegion::GetInc(int &inc0, int &inc1, int &inc2)
{
  if ( ! this->Data){
    vtkErrorMacro(<<"Data must be set or allocated.");
  }

  this->Data->GetInc(inc0, inc1, inc2);
}


int *vtkImageRegion::GetInc()
{
  if ( ! this->Data){
    vtkErrorMacro(<<"Data must be set or allocated.");
    return NULL;
  }

  return this->Data->GetInc();
}















