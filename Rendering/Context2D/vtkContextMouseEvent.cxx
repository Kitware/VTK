/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextMouseEvent.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderWindowInteractor.h" // AIX include order issues.
#include "vtkContextMouseEvent.h"

int vtkContextMouseEvent::GetModifiers() const
{
  int modifier = vtkContextMouseEvent::NO_MODIFIER;
  if (this->Interactor)
  {
    if (this->Interactor->GetAltKey() > 0)
    {
      modifier |= vtkContextMouseEvent::ALT_MODIFIER;
    }
    if (this->Interactor->GetShiftKey() > 0)
    {
      modifier |= vtkContextMouseEvent::SHIFT_MODIFIER;
    }
    if (this->Interactor->GetControlKey() > 0)
    {
      modifier |= vtkContextMouseEvent::CONTROL_MODIFIER;
    }
  }
  return modifier;
}
