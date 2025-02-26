// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkWebGPUTextureInternals.h"
#include "vtkArrayDispatch.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{
class DispatchDataWriter
{
public:
  DispatchDataWriter(
    vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration, wgpu::Texture texture)
    : WGPUConfiguration(wgpuConfiguration)
    , Texture(texture)
  {
  }

  template <typename SrcArrayType>
  void operator()(
    SrcArrayType* srcArray, uint32_t bytesPerRow, const char* description /*=nullptr*/)
  {
    using SrcType = vtk::GetAPIType<SrcArrayType>;

    const auto srcValuesIterator = vtk::DataArrayValueRange(srcArray);

    std::vector<SrcType> data;
    data.reserve(srcValuesIterator.size());
    for (const auto& value : srcValuesIterator)
    {
      data.push_back(value);
    }

    this->WGPUConfiguration->WriteTexture(this->Texture, bytesPerRow,
      data.size() * srcArray->GetDataTypeSize(), data.data(), description);
  }

private:
  vtkSmartPointer<vtkWebGPUConfiguration> WGPUConfiguration;
  wgpu::Texture Texture;
};
}

//------------------------------------------------------------------------------
void vtkWebGPUTextureInternals::Upload(vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration,
  wgpu::Texture texture, uint32_t bytesPerRow, uint32_t byteSize, const void* data,
  const char* description /*=nullptr*/)
{
  wgpuConfiguration->WriteTexture(texture, bytesPerRow, byteSize, data, description);
}

//------------------------------------------------------------------------------
void vtkWebGPUTextureInternals::UploadFromDataArray(
  vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration, wgpu::Texture texture,
  uint32_t bytesPerRow, vtkDataArray* dataArray, const char* description /*=nullptr*/)
{
  using DispatchAllTypes = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;

  DispatchDataWriter dispatchDataWriter(wgpuConfiguration, texture);

  if (!DispatchAllTypes::Execute(dataArray, dispatchDataWriter, bytesPerRow, description))
  {
    dispatchDataWriter(dataArray, bytesPerRow, description);
  }
}

//------------------------------------------------------------------------------
wgpu::TexelCopyTextureInfo vtkWebGPUTextureInternals::GetTexelCopyTextureInfo(wgpu::Texture texture)
{
  wgpu::TexelCopyTextureInfo copyTexture;
  copyTexture.aspect = wgpu::TextureAspect::All;
  copyTexture.mipLevel = 0;
  copyTexture.origin = { 0, 0, 0 };
  copyTexture.texture = texture;

  return copyTexture;
}
//------------------------------------------------------------------------------
wgpu::TexelCopyBufferLayout vtkWebGPUTextureInternals::GetDataLayout(
  wgpu::Texture texture, uint32_t bytesPerRow)
{
  wgpu::TexelCopyBufferLayout textureDataLayout;
  textureDataLayout.bytesPerRow = bytesPerRow;
  textureDataLayout.offset = 0;
  textureDataLayout.rowsPerImage = texture.GetHeight();

  return textureDataLayout;
}

VTK_ABI_NAMESPACE_END
