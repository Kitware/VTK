// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUComputeBuffer.h"
#include "vtkNew.h" // for vtk new macro
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUComputeBuffer);

//------------------------------------------------------------------------------
vtkWebGPUComputeBuffer::vtkWebGPUComputeBuffer() = default;

//------------------------------------------------------------------------------
void vtkWebGPUComputeBuffer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Group: " << this->Group << std::endl;
  os << indent << "Binding: " << this->Binding << std::endl;
  os << indent << "Mode: " << this->Mode << std::endl;

  if (this->DataPointer == nullptr)
  {
    os << indent << "VectorDataPointer: (nullptr)" << std::endl;
  }
  else
  {
    os << indent << "VectorDataPointer: " << this->DataPointer << std::endl;
  }
  os << indent << "DataArray: ";
  this->DataArray->PrintSelf(os, indent);

  os << indent << "ByteSize: " << this->ByteSize << std::endl;
  os << indent << "Label: " << this->Label << std::endl;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeBuffer::SetData(vtkDataArray* data)
{
  this->DataArray = data;
  this->ByteSize = data->GetNumberOfValues() * data->GetDataTypeSize();
}

VTK_ABI_NAMESPACE_END
