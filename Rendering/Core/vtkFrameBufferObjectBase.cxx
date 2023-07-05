// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFrameBufferObjectBase.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkFrameBufferObjectBase::vtkFrameBufferObjectBase() = default;

//------------------------------------------------------------------------------
vtkFrameBufferObjectBase::~vtkFrameBufferObjectBase() = default;

//------------------------------------------------------------------------------
void vtkFrameBufferObjectBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
