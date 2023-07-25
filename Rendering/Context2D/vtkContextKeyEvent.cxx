// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkContextKeyEvent.h"

#include "vtkRenderWindowInteractor.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkContextKeyEvent::vtkContextKeyEvent() = default;

void vtkContextKeyEvent::SetInteractor(vtkRenderWindowInteractor* interactor)
{
  this->Interactor = interactor;
}

vtkRenderWindowInteractor* vtkContextKeyEvent::GetInteractor() const
{
  return this->Interactor;
}

char vtkContextKeyEvent::GetKeyCode() const
{
  if (this->Interactor)
  {
    return this->Interactor->GetKeyCode();
  }
  else
  {
    // This should never happen, perhaps there is a better return value?
    return 0;
  }
}
VTK_ABI_NAMESPACE_END
