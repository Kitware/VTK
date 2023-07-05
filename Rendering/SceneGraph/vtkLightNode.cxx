// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLightNode.h"
#include "vtkObjectFactory.h"

#include "vtkLight.h"

//============================================================================
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLightNode);

//------------------------------------------------------------------------------
vtkLightNode::vtkLightNode() = default;

//------------------------------------------------------------------------------
vtkLightNode::~vtkLightNode() = default;

//------------------------------------------------------------------------------
void vtkLightNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
