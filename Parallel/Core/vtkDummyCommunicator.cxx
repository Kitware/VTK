// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDummyCommunicator.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDummyCommunicator);

//------------------------------------------------------------------------------
vtkDummyCommunicator::vtkDummyCommunicator()
{
  this->MaximumNumberOfProcesses = 1;
}

vtkDummyCommunicator::~vtkDummyCommunicator() = default;

void vtkDummyCommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
