// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkContextMouseEvent.h"
#include "vtkRenderWindowInteractor.h" // AIX include order issues.

VTK_ABI_NAMESPACE_BEGIN
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
VTK_ABI_NAMESPACE_END
