// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCallbackCommand.h"

//----------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkCallbackCommand::vtkCallbackCommand()
{
  this->ClientData = nullptr;
  this->Callback = nullptr;
  this->ClientDataDeleteCallback = nullptr;
  this->AbortFlagOnExecute = 0;
}

//----------------------------------------------------------------
vtkCallbackCommand::~vtkCallbackCommand()
{
  if (this->ClientDataDeleteCallback)
  {
    this->ClientDataDeleteCallback(this->ClientData);
  }
}

//----------------------------------------------------------------
void vtkCallbackCommand::Execute(vtkObject* caller, unsigned long event, void* callData)
{
  if (this->Callback)
  {
    this->Callback(caller, event, this->ClientData, callData);
    if (this->AbortFlagOnExecute)
    {
      this->AbortFlagOn();
    }
  }
}
VTK_ABI_NAMESPACE_END
