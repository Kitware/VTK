// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAbstractContextBufferId.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkAbstractObjectFactoryNewMacro(vtkAbstractContextBufferId);

//------------------------------------------------------------------------------
vtkAbstractContextBufferId::vtkAbstractContextBufferId()
{
  this->Width = 0;
  this->Height = 0;
}

//------------------------------------------------------------------------------
vtkAbstractContextBufferId::~vtkAbstractContextBufferId() = default;

//------------------------------------------------------------------------------
void vtkAbstractContextBufferId::ReleaseGraphicsResources() {}

//------------------------------------------------------------------------------
void vtkAbstractContextBufferId::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
