// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDummyGPUInfoList.h"

#include "vtkGPUInfoListArray.h"

#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDummyGPUInfoList);

//------------------------------------------------------------------------------
// Description:
// Build the list of vtkInfoGPU if not done yet.
// \post probed: IsProbed()
void vtkDummyGPUInfoList::Probe()
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
vtkDummyGPUInfoList::vtkDummyGPUInfoList() = default;

//------------------------------------------------------------------------------
vtkDummyGPUInfoList::~vtkDummyGPUInfoList() = default;

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkDummyGPUInfoList::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "OpenGL", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
void vtkDummyGPUInfoList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
