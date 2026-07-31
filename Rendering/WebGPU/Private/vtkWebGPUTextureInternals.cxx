// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkWebGPUTextureInternals.h"
#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{
class DispatchDataWriter
{
public:
  DispatchDataWriter(vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration, WGPUTexture texture)
    : WGPUConfiguration(wgpuConfiguration)
    , Texture(texture)
  {
  }

  template <typename SrcArrayType>
  void operator()(
    SrcArrayType* srcArray, std::uint32_t bytesPerRow, const char* description /*=nullptr*/)
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
      data.size() * srcArray->GetDataTypeSize(), data.data(), /*srcOffset=*/0,
      /*dstOffset=*/{ 0, 0, 0 },
      /*dstMipLevel=*/0, description);
  }

private:
  vtkSmartPointer<vtkWebGPUConfiguration> WGPUConfiguration;
  WGPUTexture Texture;
};
}

//------------------------------------------------------------------------------
void vtkWebGPUTextureInternals::Upload(vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration,
  WGPUTexture texture, std::uint32_t bytesPerRow, std::uint32_t byteSize, const void* data,
  const char* description /*=nullptr*/)
{
  wgpuConfiguration->WriteTexture(texture, bytesPerRow, byteSize, data, /*srcOffset=*/0,
    /*dstOffset=*/{ 0, 0, 0 }, /*dstMipLevel=*/0, description);
}

//------------------------------------------------------------------------------
void vtkWebGPUTextureInternals::UploadFromDataArray(
  vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration, WGPUTexture texture,
  std::uint32_t bytesPerRow, vtkDataArray* dataArray, const char* description /*=nullptr*/)
{
  using Dispatcher = vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::AllArrays>;

  DispatchDataWriter dispatchDataWriter(wgpuConfiguration, texture);

  if (!Dispatcher::Execute(dataArray, dispatchDataWriter, bytesPerRow, description))
  {
    dispatchDataWriter(dataArray, bytesPerRow, description);
  }
}

//------------------------------------------------------------------------------
WGPUTexelCopyTextureInfo vtkWebGPUTextureInternals::GetTexelCopyTextureInfo(
  WGPUTexture texture, WGPUOrigin3D origin /*= { 0, 0, 0 }*/, std::uint32_t mipLevel /*= 0*/)
{
  WGPUTexelCopyTextureInfo copyTexture = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
  copyTexture.aspect = WGPUTextureAspect_All;
  copyTexture.mipLevel = mipLevel;
  copyTexture.origin = origin;
  copyTexture.texture = texture;

  return copyTexture;
}
//------------------------------------------------------------------------------
WGPUTexelCopyBufferLayout vtkWebGPUTextureInternals::GetDataLayout(
  WGPUTexture texture, std::uint32_t bytesPerRow, std::uint32_t srcOffset /*= 0*/)
{
  WGPUTexelCopyBufferLayout textureDataLayout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
  textureDataLayout.bytesPerRow = bytesPerRow;
  textureDataLayout.offset = srcOffset;
  textureDataLayout.rowsPerImage = wgpuTextureGetHeight(texture);

  return textureDataLayout;
}

VTK_ABI_NAMESPACE_END
