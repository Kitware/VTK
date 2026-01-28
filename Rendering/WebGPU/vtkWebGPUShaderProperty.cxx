// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUShaderProperty.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUShaderProperty);

vtkWebGPUShaderProperty::vtkWebGPUShaderProperty() = default;

vtkWebGPUShaderProperty::~vtkWebGPUShaderProperty() = default;

vtkOverrideAttribute* vtkWebGPUShaderProperty::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", nullptr);
  return renderingBackendAttribute;
}

VTK_ABI_NAMESPACE_END
