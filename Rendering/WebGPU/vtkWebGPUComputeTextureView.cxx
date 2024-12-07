// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUComputeTextureView.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUComputeTextureView);

//------------------------------------------------------------------------------
vtkWebGPUComputeTextureView::vtkWebGPUComputeTextureView() = default;

//------------------------------------------------------------------------------
vtkWebGPUComputeTextureView::~vtkWebGPUComputeTextureView() = default;

//------------------------------------------------------------------------------
void vtkWebGPUComputeTextureView::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << indent << "AssociatedTextureIndex: " << this->AssociatedTextureIndex << std::endl;
}

VTK_ABI_NAMESPACE_END
