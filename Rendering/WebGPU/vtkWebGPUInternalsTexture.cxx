// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUInternalsTexture.h"
#include "vtkArrayDispatch.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{
class DispatchDataWriter
{
public:
  DispatchDataWriter(wgpu::Device device, wgpu::ImageCopyTexture imageCopyTexture,
    wgpu::TextureDataLayout dataLayout, wgpu::Extent3D extents)
    : Device(device)
    , ImageCopyTexture(imageCopyTexture)
    , DataLayout(dataLayout)
    , Extents(extents)
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

    this->Device.GetQueue().WriteTexture(&this->ImageCopyTexture, data.data(),
      data.size() * srcArray->GetDataTypeSize(), &this->DataLayout, &this->Extents);
  }

private:
  wgpu::Device Device;
  wgpu::ImageCopyTexture ImageCopyTexture;
  wgpu::TextureDataLayout DataLayout;
  wgpu::Extent3D Extents;
};
}

//------------------------------------------------------------------------------
wgpu::Texture vtkWebGPUInternalsTexture::CreateATexture(const wgpu::Device& device,
  wgpu::Extent3D extents, wgpu::TextureDimension dimension, wgpu::TextureFormat format,
  wgpu::TextureUsage usage, int mipLevelCount, std::string label)
{
  // Preparing the texture descriptor
  wgpu::TextureDescriptor textureDescriptor;
  textureDescriptor.dimension = dimension;
  textureDescriptor.format = format;
  textureDescriptor.size = extents;
  textureDescriptor.mipLevelCount = mipLevelCount;
  textureDescriptor.nextInChain = nullptr;
  textureDescriptor.sampleCount = 1;
  textureDescriptor.usage = usage;
  textureDescriptor.viewFormatCount = 0;
  textureDescriptor.viewFormats = nullptr;
  textureDescriptor.label = label.c_str();

  // Creating the texture
  return device.CreateTexture(&textureDescriptor);
}

//------------------------------------------------------------------------------
wgpu::TextureView vtkWebGPUInternalsTexture::CreateATextureView(const wgpu::Device& device,
  wgpu::Texture texture, wgpu::TextureViewDimension dimension, wgpu::TextureAspect aspect,
  wgpu::TextureFormat format, int baseMipLevel, int mipLevelCount, std::string label)
{
  wgpu::TextureViewDimension textureViewDimension = dimension;
  // Creating a "full" view of the texture
  wgpu::TextureViewDescriptor textureViewDescriptor;
  textureViewDescriptor.arrayLayerCount = 1;
  textureViewDescriptor.aspect = aspect;
  textureViewDescriptor.baseArrayLayer = 0;
  textureViewDescriptor.baseMipLevel = baseMipLevel;
  textureViewDescriptor.dimension = textureViewDimension;
  textureViewDescriptor.format = format;
  textureViewDescriptor.label = label.c_str();
  textureViewDescriptor.mipLevelCount = mipLevelCount;
  textureViewDescriptor.nextInChain = nullptr;

  return texture.CreateView(&textureViewDescriptor);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsTexture::Upload(const wgpu::Device& device, wgpu::Texture texture,
  uint32_t bytesPerRow, uint32_t byteSize, const void* data)
{
  wgpu::ImageCopyTexture copyTexture = vtkWebGPUInternalsTexture::GetImageCopyTexture(texture);

  wgpu::TextureDataLayout textureDataLayout =
    vtkWebGPUInternalsTexture::GetDataLayout(texture, bytesPerRow);

  wgpu::Extent3D textureExtents = { texture.GetWidth(), texture.GetHeight(),
    texture.GetDepthOrArrayLayers() };

  device.GetQueue().WriteTexture(&copyTexture, data, byteSize, &textureDataLayout, &textureExtents);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsTexture::UploadFromDataArray(
  const wgpu::Device& device, wgpu::Texture texture, uint32_t bytesPerRow, vtkDataArray* dataArray)
{
  using DispatchAllTypes = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;

  wgpu::ImageCopyTexture imageCopyTexture = vtkWebGPUInternalsTexture::GetImageCopyTexture(texture);
  wgpu::TextureDataLayout dataLayout =
    vtkWebGPUInternalsTexture::GetDataLayout(texture, bytesPerRow);

  wgpu::Extent3D extents = { texture.GetWidth(), texture.GetHeight(),
    texture.GetDepthOrArrayLayers() };

  DispatchDataWriter dispatchDataWriter(device, imageCopyTexture, dataLayout, extents);

  if (!DispatchAllTypes::Execute(dataArray, dispatchDataWriter))
  {
    dispatchDataWriter(dataArray);
  }
}

//------------------------------------------------------------------------------
wgpu::ImageCopyTexture vtkWebGPUInternalsTexture::GetImageCopyTexture(wgpu::Texture texture)
{
  wgpu::ImageCopyTexture copyTexture;
  copyTexture.aspect = wgpu::TextureAspect::All;
  copyTexture.mipLevel = 0;
  copyTexture.origin = { 0, 0, 0 };
  copyTexture.texture = texture;

  return copyTexture;
}
//------------------------------------------------------------------------------
wgpu::TextureDataLayout vtkWebGPUInternalsTexture::GetDataLayout(
  wgpu::Texture texture, uint32_t bytesPerRow)
{
  wgpu::TextureDataLayout textureDataLayout;
  textureDataLayout.nextInChain = nullptr;
  textureDataLayout.bytesPerRow = bytesPerRow;
  textureDataLayout.offset = 0;
  textureDataLayout.rowsPerImage = texture.GetHeight();

  return textureDataLayout;
}

VTK_ABI_NAMESPACE_END
