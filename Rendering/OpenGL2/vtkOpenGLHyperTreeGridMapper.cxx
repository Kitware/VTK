// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLHyperTreeGridMapper.h"

#include "vtkCompositePolyDataMapper.h" // For Mapper3D
#include "vtkObjectFactory.h"           // For the macro
#include "vtkOverrideAttribute.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLHyperTreeGridMapper);

//------------------------------------------------------------------------------
vtkOpenGLHyperTreeGridMapper::vtkOpenGLHyperTreeGridMapper()
{
  this->Mapper = vtkSmartPointer<vtkCompositePolyDataMapper>::New();
}

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkOpenGLHyperTreeGridMapper::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "OpenGL", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
void vtkOpenGLHyperTreeGridMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
