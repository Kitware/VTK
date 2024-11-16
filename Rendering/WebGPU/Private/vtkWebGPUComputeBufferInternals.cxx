// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkWebGPUComputeBufferInternals.h"
#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"

namespace
{
class DispatchDataWriter
{
public:
  DispatchDataWriter(vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration, wgpu::Buffer buffer,
    vtkIdType byteOffset)
    : WGPUConfiguration(wgpuConfiguration)
    , Buffer(buffer)
    , ByteOffset(byteOffset)
  {
  }

  template <typename SrcArrayType>
  void operator()(SrcArrayType* srcArray, const char* description)
  {
    using SrcType = vtk::GetAPIType<SrcArrayType>;

    const auto srcValuesIterator = vtk::DataArrayValueRange(srcArray);

    std::vector<SrcType> data;
    data.reserve(srcValuesIterator.size());
    for (const auto& value : srcValuesIterator)
    {
      data.push_back(value);
    }
    this->WGPUConfiguration->WriteBuffer(this->Buffer, this->ByteOffset, data.data(),
      data.size() * srcArray->GetDataTypeSize(), description);
  }

private:
  vtkSmartPointer<vtkWebGPUConfiguration> WGPUConfiguration;
  wgpu::Buffer Buffer;
  vtkIdType ByteOffset;
};
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeBufferInternals::UploadFromDataArray(
  vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration, wgpu::Buffer buffer,
  vtkDataArray* dataArray, const char* description /*=nullptr*/)
{
  UploadFromDataArray(wgpuConfiguration, buffer, 0, dataArray, description);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeBufferInternals::UploadFromDataArray(
  vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration, wgpu::Buffer buffer,
  vtkIdType byteOffset, vtkDataArray* dataArray, const char* description /*=nullptr*/)
{
  using DispatchAllTypes = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;

  DispatchDataWriter dispatchDataWriter(wgpuConfiguration, buffer, byteOffset);

  if (!DispatchAllTypes::Execute(dataArray, dispatchDataWriter, description))
  {
    dispatchDataWriter(dataArray, description);
  }
}
