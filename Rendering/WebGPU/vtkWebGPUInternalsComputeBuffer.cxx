// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUInternalsComputeBuffer.h"
#include "vtkArrayDispatch.h"

class DispatchDataWriter
{
public:
  DispatchDataWriter(wgpu::Device device, wgpu::Buffer buffer)
    : Device(device)
    , Buffer(buffer)
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
      Buffer, 0, data.data(), data.size() * srcArray->GetDataTypeSize());
  }

private:
  wgpu::Device Device;
  wgpu::Buffer Buffer;
};

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputeBuffer::UploadFromDataArray(
  wgpu::Device device, wgpu::Buffer buffer, vtkDataArray* dataArray)
{
  using DispatchAllTypes = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;

  DispatchDataWriter dispatchDataWriter(device, buffer);

  if (!DispatchAllTypes::Execute(dataArray, dispatchDataWriter))
  {
    dispatchDataWriter(dataArray);
  }
}
