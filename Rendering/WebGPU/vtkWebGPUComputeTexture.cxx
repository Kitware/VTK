// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUComputeTexture.h"
#include "vtkNew.h"           // for the new macro
#include "vtkObjectFactory.h" // for the new macro

#include <vector>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUComputeTexture);

//------------------------------------------------------------------------------
vtkWebGPUComputeTexture::vtkWebGPUComputeTexture() = default;

//------------------------------------------------------------------------------
vtkWebGPUComputeTexture::~vtkWebGPUComputeTexture() = default;

//------------------------------------------------------------------------------
void vtkWebGPUComputeTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << indent << "ByteSize: " << this->ByteSize << std::endl;

  os << indent << "DataType: " << this->DataType << std::endl;
  os << indent << "DataPointer: " << this->DataPointer << std::endl;
  os << indent << "DataArray: " << this->DataArray << std::endl;
  os << indent << "Label: " << this->Label << std::endl;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeTexture::SetData(vtkDataArray* data)
{
  this->DataArray = data;
  this->ByteSize = data->GetNumberOfValues() * data->GetDataTypeSize();
}

VTK_ABI_NAMESPACE_END
