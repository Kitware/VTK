/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleSwitchBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkInteractorStyleSwitchBase.h"

#include "vtkGraphicsFactory.h"
#include "vtkDebugLeaks.h"

// This is largely here to confirm the approach works, and will be replaced
// with standard factory override logic in the modularized source tree.
//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkInteractorStyleSwitchBase);

//----------------------------------------------------------------------
vtkInteractorStyleSwitchBase *vtkInteractorStyleSwitchBase::New()
{
  // First try to create the object from the vtkGraphicsFactory
  vtkObject* ret =
    vtkGraphicsFactory::CreateInstance("vtkInteractorStyleSwitchBase");
  if (ret)
    {
    return static_cast<vtkInteractorStyleSwitchBase *>(ret);
    }
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkInteractorStyleSwitchBase");
#endif
  return new vtkInteractorStyleSwitchBase;
}

//----------------------------------------------------------------------------
vtkInteractorStyleSwitchBase::vtkInteractorStyleSwitchBase()
{
}

//----------------------------------------------------------------------------
vtkInteractorStyleSwitchBase::~vtkInteractorStyleSwitchBase()
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitchBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
