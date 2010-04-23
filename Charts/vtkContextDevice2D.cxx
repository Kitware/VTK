/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextDevice2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#include "vtkContextDevice2D.h"

#include "vtkObjectFactory.h"
#include <assert.h>

//-----------------------------------------------------------------------------
//vtkStandardNewMacro(vtkContextDevice2D);

vtkContextDevice2D::vtkContextDevice2D()
{
  this->Geometry[0] = 0;
  this->Geometry[1] = 0;
  this->BufferId=0;
}

//-----------------------------------------------------------------------------
vtkContextDevice2D::~vtkContextDevice2D()
{
}

// ----------------------------------------------------------------------------
bool vtkContextDevice2D::GetBufferIdMode() const
{
  return this->BufferId!=0;
}
  
// ----------------------------------------------------------------------------
void vtkContextDevice2D::BufferIdModeBegin(
  vtkAbstractContextBufferId *bufferId)
{
  assert("pre: not_yet" && !this->GetBufferIdMode());
  assert("pre: bufferId_exists" && bufferId!=0);
  
  this->BufferId=bufferId;
  
  assert("post: started" && this->GetBufferIdMode());
}
  
// ----------------------------------------------------------------------------
void vtkContextDevice2D::BufferIdModeEnd()
{
  assert("pre: started" && this->GetBufferIdMode());
  
  this->BufferId=0;
  
  assert("post: done" && !this->GetBufferIdMode());
}

//-----------------------------------------------------------------------------
void vtkContextDevice2D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
