// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUTextureView.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUTextureView);

//------------------------------------------------------------------------------
vtkWebGPUTextureView::vtkWebGPUTextureView() = default;

//------------------------------------------------------------------------------
vtkWebGPUTextureView::~vtkWebGPUTextureView() = default;

//------------------------------------------------------------------------------
void vtkWebGPUTextureView::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "BaseMipLevel: " << this->BaseMipLevel << std::endl;
  os << indent << "MipLevelCount: " << this->MipLevelCount << std::endl;
  os << indent << "Group: " << this->Group << std::endl;
  os << indent << "Binding: " << this->Binding << std::endl;

  os << indent << "Aspect: " << this->Aspect << std::endl;
  os << indent << "Dimension: " << this->Dimension << std::endl;
  os << indent << "Format: " << this->Format << std::endl;
  os << indent << "Mode: " << this->Mode << std::endl;
  os << indent << "Label: " << this->Label << std::endl;
}

VTK_ABI_NAMESPACE_END
