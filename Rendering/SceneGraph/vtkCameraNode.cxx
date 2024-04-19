// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCameraNode.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"

//============================================================================
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCameraNode);

//------------------------------------------------------------------------------
vtkCameraNode::vtkCameraNode() = default;

//------------------------------------------------------------------------------
vtkCameraNode::~vtkCameraNode() = default;

//------------------------------------------------------------------------------
void vtkCameraNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
