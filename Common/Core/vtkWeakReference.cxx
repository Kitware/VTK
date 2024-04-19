// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWeakReference.h"
#include "vtkObjectFactory.h"
#include "vtkWeakPointer.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWeakReference);

//------------------------------------------------------------------------------
vtkWeakReference::vtkWeakReference() = default;

//------------------------------------------------------------------------------
vtkWeakReference::~vtkWeakReference() = default;

//------------------------------------------------------------------------------
void vtkWeakReference::Set(vtkObject* object)
{
  this->Object = object;
}

//------------------------------------------------------------------------------
vtkObject* vtkWeakReference::Get()
{
  return this->Object;
}
VTK_ABI_NAMESPACE_END
