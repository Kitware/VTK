// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSynchronizableActors.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkSynchronizableActors::vtkSynchronizableActors() = default;

//------------------------------------------------------------------------------
vtkSynchronizableActors::~vtkSynchronizableActors() = default;

//------------------------------------------------------------------------------
void vtkSynchronizableActors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
