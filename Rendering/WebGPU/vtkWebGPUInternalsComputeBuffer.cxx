// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUInternalsComputeBuffer.h"
#include "vtkArrayDispatch.h"

namespace
{
class DispatchDataWriter
{
public:
  DispatchDataWriter(wgpu::Device device, wgpu::Buffer buffer, vtkIdType byteOffset)
    : Device(device)
    , Buffer(buffer)
    , ByteOffset(byteOffset)
  {
  }

  template <typename SrcArrayType>
  void operator()(SrcArrayType* srcArray)
  {
    using SrcType = vtk::GetAPIType<SrcArrayType>;

    const auto srcValuesIterator = vtk::DataArrayValueRange(srcArray);

    std::vector<SrcType> data;
    data.reserve(srcValuesIterator.size());
    for (const auto& value : srcValuesIterator)
    {
      data.push_back(value);
    }

    this->Device.GetQueue().WriteBuffer(
      this->Buffer, this->ByteOffset, data.data(), data.size() * srcArray->GetDataTypeSize());
  }

private:
  wgpu::Device Device;
  wgpu::Buffer Buffer;
  vtkIdType ByteOffset;
};
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputeBuffer::UploadFromDataArray(
  wgpu::Device device, wgpu::Buffer buffer, vtkDataArray* dataArray)
{
  UploadFromDataArray(device, buffer, 0, dataArray);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputeBuffer::UploadFromDataArray(
  wgpu::Device device, wgpu::Buffer buffer, vtkIdType byteOffset, vtkDataArray* dataArray)
{
  using DispatchAllTypes = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;

  DispatchDataWriter dispatchDataWriter(device, buffer, byteOffset);

  if (!DispatchAllTypes::Execute(dataArray, dispatchDataWriter))
  {
    dispatchDataWriter(dataArray);
  }
}
