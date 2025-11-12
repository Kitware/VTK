// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkWebGPURenderTextureDeviceResource
 * @brief
 */
#ifndef vtkWebGPURenderTextureDeviceResource_h
#define vtkWebGPURenderTextureDeviceResource_h
#include "vtkDataArray.h"
#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkWebGPUTextureDeviceResource.h"
#include "vtk_wgpu.h" // for webgpu

#include <cstdint>
#include <webgpu/webgpu_cpp.h>

VTK_ABI_NAMESPACE_BEGIN
class vtkWindow;
class vtkImageData;
class vtkWebGPUConfiguration;
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPURenderTextureDeviceResource
  : public vtkWebGPUTextureDeviceResource
{
public:
  vtkTypeMacro(vtkWebGPURenderTextureDeviceResource, vtkWebGPUTextureDeviceResource);
  static vtkWebGPURenderTextureDeviceResource* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Release graphics resources associated with this texture
   */
  void ReleaseGraphicsResources(vtkWindow* window);

  enum class AddressMode
  {
    UNDEFINED = 0,
    CLAMP_TO_EDGE,
    REPEAT,
    MIRROR_REPEAT
  };

  enum class FilterMode
  {
    UNDEFINED = 0,
    NEAREST,
    LINEAR
  };

  enum class SamplerMode
  {
    UNDEFINED = 0,
    FILTERING,
    NON_FILTERING,
    COMPARISON
  };

  enum class CompareFunction
  {
    UNDEFINED = 0,
    NEVER,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
    EQUAL,
    NOT_EQUAL,
    ALWAYS
  };

  ///@{
  /**
   * Get/set the address mode for U, V and W directions
   */
  vtkGetEnumMacro(AddressModeU, AddressMode);
  vtkSetEnumMacro(AddressModeU, AddressMode);
  vtkGetEnumMacro(AddressModeV, AddressMode);
  vtkSetEnumMacro(AddressModeV, AddressMode);
  vtkGetEnumMacro(AddressModeW, AddressMode);
  vtkSetEnumMacro(AddressModeW, AddressMode);
  ///@}

  ///@{
  /**
   * Get/set the filter mode for magnification, minification and mipmap
   */
  vtkGetEnumMacro(MagFilter, FilterMode);
  vtkSetEnumMacro(MagFilter, FilterMode);
  vtkGetEnumMacro(MinFilter, FilterMode);
  vtkSetEnumMacro(MinFilter, FilterMode);
  vtkGetEnumMacro(MipmapFilter, FilterMode);
  vtkSetEnumMacro(MipmapFilter, FilterMode);
  ///@}

  ///@{
  /**
   * Get/set the sampler type for the texture
   */
  vtkGetEnumMacro(SamplerBindingType, SamplerMode);
  vtkSetEnumMacro(SamplerBindingType, SamplerMode);
  ///@}

  ///@{
  /**
   * Get/set the compare function for depth textures. Used when SamplerBindingType is COMPARISON.
   */
  vtkGetEnumMacro(CompareFunc, CompareFunction);
  vtkSetEnumMacro(CompareFunc, CompareFunction);
  ///@}

  ///@{
  /**
   * Get/set the LOD min and max clamp values
   */
  vtkGetMacro(LODMinClamp, float);
  vtkSetMacro(LODMinClamp, float);
  vtkGetMacro(LODMaxClamp, float);
  vtkSetMacro(LODMaxClamp, float);
  ///@}

  ///@{
  /**
   * Get/set the maximum anisotropy level
   */
  vtkGetMacro(MaxAnisotropy, std::uint16_t);
  vtkSetMacro(MaxAnisotropy, std::uint16_t);
  ///@}

  ///@{
  /**
   * Get/set the sample count for multi-sampled textures
   */
  vtkGetMacro(SampleCount, std::uint32_t);
  vtkSetMacro(SampleCount, std::uint32_t);
  ///@}

  /**
   * Send the texture data to the WebGPU device
   * @param dataPlanes The data planes to upload to the texture
   * @param wgpuConfiguration The WebGPU configuration to use for uploading
   * @param cubeMap Whether the texture is a cube map
   */
  void SendToWebGPUDevice(
    std::vector<void*> dataPlanes, vtkWebGPUConfiguration* wgpuConfiguration, bool cubeMap = false);

  /**
   * Create a sampler bind group layout entry.
   * @param binding The binding index.
   * @param visibility The shader stage visibility.
   * @return The created bind group layout entry.
   */
  wgpu::BindGroupLayoutEntry MakeSamplerBindGroupLayoutEntry(
    std::uint32_t binding, wgpu::ShaderStage visibility);

  /**
   * Create a sampler bind group entry.
   * @param binding The binding index.
   * @return The created bind group entry.
   */
  wgpu::BindGroupEntry MakeSamplerBindGroupEntry(std::uint32_t binding);

  /**
   * Create a texture view bind group layout entry.
   * @param binding The binding index.
   * @param visibility The shader stage visibility.
   * @return The created bind group layout entry.
   */
  wgpu::BindGroupLayoutEntry MakeTextureViewBindGroupLayoutEntry(
    std::uint32_t binding, wgpu::ShaderStage visibility);

  /**
   * Create a texture view bind group entry.
   * @param binding The binding index.
   * @return The created bind group entry.
   */
  wgpu::BindGroupEntry MakeTextureViewBindGroupEntry(std::uint32_t binding);

  ///@{
  /**
   * Get/set the label for the texture.
   */
  vtkSetStdStringFromCharMacro(Label);
  vtkSetMacro(Label, std::string);
  vtkGetCharFromStdStringMacro(Label);
  ///@}

  static const char* GetTextureSampleTypeString(TextureSampleType type);
  static wgpu::FilterMode GetWebGPUFilterMode(FilterMode mode);
  static wgpu::MipmapFilterMode GetWGPUMipMapFilterMode(FilterMode mode);
  static wgpu::AddressMode GetWebGPUAddressMode(AddressMode mode);
  static wgpu::SamplerBindingType GetWebGPUSamplerBindingType(SamplerMode mode);
  static wgpu::CompareFunction GetWebGPUCompareFunction(CompareFunction mode);

protected:
  vtkWebGPURenderTextureDeviceResource();
  ~vtkWebGPURenderTextureDeviceResource() override;

private:
  vtkWebGPURenderTextureDeviceResource(const vtkWebGPURenderTextureDeviceResource&) = delete;
  void operator=(const vtkWebGPURenderTextureDeviceResource&) = delete;

  AddressMode AddressModeU = AddressMode::UNDEFINED;
  AddressMode AddressModeV = AddressMode::UNDEFINED;
  AddressMode AddressModeW = AddressMode::UNDEFINED;

  FilterMode MagFilter = FilterMode::UNDEFINED;
  FilterMode MinFilter = FilterMode::UNDEFINED;
  FilterMode MipmapFilter = FilterMode::UNDEFINED;

  SamplerMode SamplerBindingType = SamplerMode::UNDEFINED;
  CompareFunction CompareFunc = CompareFunction::UNDEFINED;

  float LODMinClamp = 0.0f;
  float LODMaxClamp = 32.0f;
  std::uint16_t MaxAnisotropy = 1;

  std::uint32_t SampleCount = 1;
  std::uint32_t BaseMipLevel = 0;

  std::string Label;

  wgpu::TextureDescriptor TextureDescriptor;
  wgpu::Texture Texture;
  wgpu::SamplerDescriptor SamplerDescriptor;
  wgpu::Sampler Sampler;
  wgpu::TextureViewDescriptor TextureViewDescriptor;
  wgpu::TextureView TextureView;
};

VTK_ABI_NAMESPACE_END
#endif
