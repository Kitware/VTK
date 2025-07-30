// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK_DEPRECATED_IN_9_6_0
#define VTK_DEPRECATION_LEVEL 0

#include "vtkOldStyleCallbackCommand.h"

#include "vtkObject.h"
#include "vtkSetGet.h"

#include <cctype>
#include <cstring>

//----------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkOldStyleCallbackCommand::vtkOldStyleCallbackCommand()
{
  this->ClientData = nullptr;
  this->Callback = nullptr;
  this->ClientDataDeleteCallback = nullptr;
}

vtkOldStyleCallbackCommand::~vtkOldStyleCallbackCommand()
{
  if (this->ClientDataDeleteCallback)
  {
    this->ClientDataDeleteCallback(this->ClientData);
  }
}

void vtkOldStyleCallbackCommand::Execute(vtkObject*, unsigned long, void*)
{
  if (this->Callback)
  {
    this->Callback(this->ClientData);
  }
}
VTK_ABI_NAMESPACE_END
