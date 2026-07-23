// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariGPUInfoList.h"

#include "vtkGPUInfoListArray.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAnariGPUInfoList);

//------------------------------------------------------------------------------
void vtkAnariGPUInfoList::Probe()
{
  if (!this->Probed)
  {
    this->Probed = true;
    this->Array = new vtkGPUInfoListArray;
    this->Array->v.resize(0); // no GPU.
  }
  assert("post: probed" && this->IsProbed());
}

//------------------------------------------------------------------------------
vtkAnariGPUInfoList::vtkAnariGPUInfoList() = default;

//------------------------------------------------------------------------------
vtkAnariGPUInfoList::~vtkAnariGPUInfoList() = default;

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkAnariGPUInfoList::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "ANARI", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
void vtkAnariGPUInfoList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
