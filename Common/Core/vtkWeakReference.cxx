/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWeakReference.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWeakReference.h"
#include "vtkWeakPointer.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkWeakReference)


//----------------------------------------------------------------------------
vtkWeakReference::vtkWeakReference()
{
}

//----------------------------------------------------------------------------
vtkWeakReference::~vtkWeakReference()
{
}

//----------------------------------------------------------------------------
void vtkWeakReference::Set(vtkObject *object)
{
  this->Object = object;
}

//----------------------------------------------------------------------------
vtkObject* vtkWeakReference::Get()
{
  return this->Object;
}
