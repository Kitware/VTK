/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrameBufferObjectBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFrameBufferObjectBase.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkFrameBufferObjectBase::vtkFrameBufferObjectBase()
{
}

//----------------------------------------------------------------------------
vtkFrameBufferObjectBase::~vtkFrameBufferObjectBase()
{
}

// ----------------------------------------------------------------------------
void vtkFrameBufferObjectBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
