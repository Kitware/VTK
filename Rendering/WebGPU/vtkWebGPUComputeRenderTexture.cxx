// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUComputeRenderTexture.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPUComputePass.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUComputeRenderTexture);

//------------------------------------------------------------------------------
vtkWebGPUComputeRenderTexture::vtkWebGPUComputeRenderTexture() = default;

//------------------------------------------------------------------------------
vtkWebGPUComputeRenderTexture::~vtkWebGPUComputeRenderTexture() = default;

//------------------------------------------------------------------------------
void vtkWebGPUComputeRenderTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Aspect: " << this->Aspect << std::endl;
  os << indent << "Type: " << this->Type << std::endl;

  os << indent << "WebGPUTexture: " << this->WebGPUTexture.Get() << std::endl;
  os << indent << "Associated compute pass: ";

  this->AssociatedComputePass->PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkWeakPointer<vtkWebGPUComputePass> vtkWebGPUComputeRenderTexture::GetAssociatedComputePass()
{
  return this->AssociatedComputePass;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeRenderTexture::SetAssociatedComputePass(
  vtkWeakPointer<vtkWebGPUComputePass> computePass)
{
  this->AssociatedComputePass = computePass;
}

VTK_ABI_NAMESPACE_END
