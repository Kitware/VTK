// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPProgressObserver.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSMPProgressObserver);

vtkSMPProgressObserver::vtkSMPProgressObserver() = default;

vtkSMPProgressObserver::~vtkSMPProgressObserver() = default;

void vtkSMPProgressObserver::UpdateProgress(double progress)
{
  vtkProgressObserver* observer = this->Observers.Local();
  observer->UpdateProgress(progress);
}

void vtkSMPProgressObserver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
