// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnariGPUVolumeRayCastMapper.h"

#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"

VTK_ABI_NAMESPACE_BEGIN

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariGPUVolumeRayCastMapper);

// ----------------------------------------------------------------------------
void vtkAnariGPUVolumeRayCastMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
vtkOverrideAttribute* vtkAnariGPUVolumeRayCastMapper::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "ANARI", nullptr);
  return renderingBackendAttribute;
}

VTK_ABI_NAMESPACE_END
