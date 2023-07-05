// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkProcess.h"
#include "vtkMultiProcessController.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkProcess::vtkProcess()
{
  this->Controller = nullptr;
  this->ReturnValue = 0;
}

//------------------------------------------------------------------------------
vtkProcess::~vtkProcess()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
vtkMultiProcessController* vtkProcess::GetController()
{
  return this->Controller;
}

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkProcess, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
int vtkProcess::GetReturnValue()
{
  return this->ReturnValue;
}

//------------------------------------------------------------------------------
void vtkProcess::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ReturnValue: " << this->ReturnValue << endl;
  os << indent << "Controller: ";
  if (this->Controller)
  {
    os << endl;
    this->Controller->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}
VTK_ABI_NAMESPACE_END
