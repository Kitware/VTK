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
vtkImageRegion::~vtkImageRegion()
{
  if (this->Data)
    this->Data->Delete();
}


//----------------------------------------------------------------------------
// Description:
// This method allocates memory for the tile data.  
// Returns 1 if sucessful, 0 if not.
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
// You can set the data object explicitly
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
// This Method returns a pointer at a location in the tile.
// It just passes the request to its data object.
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
// This lets the caller march through the data memory quickly using 
// pointer arithmatic.
void vtkImageRegion::GetInc(int &inc0, int &inc1, int &inc2)
{
  if ( ! this->Data){
    vtkErrorMacro(<<"Data must be set or allocated.");
  }

  this->Data->GetInc(inc0, inc1, inc2);
}


//----------------------------------------------------------------------------
// Description:
// This method returns an array containing the increments between pixels, 
// rows, and images.  This lets the caller march through the data memory 
// quickly using pointer arithmatic.
int *vtkImageRegion::GetInc()
{
  if ( ! this->Data){
    vtkErrorMacro(<<"Data must be set or allocated.");
    return NULL;
  }

  return this->Data->GetInc();
}















