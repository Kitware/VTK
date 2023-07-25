// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkEventForwarderCommand.h"
#include "vtkObject.h"

//----------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkEventForwarderCommand::vtkEventForwarderCommand()
{
  this->Target = nullptr;
}

//----------------------------------------------------------------
void vtkEventForwarderCommand::Execute(vtkObject*, unsigned long event, void* call_data)
{
  if (this->Target)
  {
    this->Target->InvokeEvent(event, call_data);
  }
}
VTK_ABI_NAMESPACE_END
