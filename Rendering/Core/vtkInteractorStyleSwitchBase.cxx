// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkInteractorStyleSwitchBase.h"

#include "vtkObjectFactory.h"

// This is largely here to confirm the approach works, and will be replaced
// with standard factory override logic in the modularized source tree.
//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkInteractorStyleSwitchBase);

//------------------------------------------------------------------------------
vtkInteractorStyleSwitchBase::vtkInteractorStyleSwitchBase() = default;

//------------------------------------------------------------------------------
vtkInteractorStyleSwitchBase::~vtkInteractorStyleSwitchBase() = default;

//------------------------------------------------------------------------------
vtkRenderWindowInteractor* vtkInteractorStyleSwitchBase::GetInteractor()
{
  static bool warned = false;
  if (!warned && strcmp(this->GetClassName(), "vtkInteractorStyleSwitchBase") == 0)
  {
    vtkWarningMacro("Warning: Link to vtkInteractionStyle for default style selection.");
    warned = true;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkInteractorStyleSwitchBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
