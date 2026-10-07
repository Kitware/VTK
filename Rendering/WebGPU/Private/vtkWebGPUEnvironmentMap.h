// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkWebGPUEnvironmentMap
 * @brief Generates the image based lighting textures of a WebGPU renderer.
 */

#ifndef vtkWebGPUEnvironmentMap_h
#define vtkWebGPUEnvironmentMap_h

#include "Private/vtkWebGPUHandle.h" // for vtkWebGPU::Handle
#include "vtkType.h"                 // for vtkMTimeType

#include <cstdint> // for std::uint32_t
#include <string>  // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkWebGPUConfiguration;

class vtkWebGPUEnvironmentMap
{
public:
  /**
   * The environment texture the lighting textures are generated from.
   */
  struct Source
  {
    WGPUTextureView View = nullptr;
    bool IsCubeMap = false;
    std::uint32_t Height = 0;
    // True when View holds sRGB encoded values that the generation shaders must linearize
    // before they filter them.
    bool ConvertToLinear = false;
    // Orthonormal basis of the environment, from vtkRenderer::GetEnvironmentRight() and
    // GetEnvironmentUp(), with Front their cross product. Sample directions are rotated into
    // this basis so the environment keeps its orientation whatever the camera does.
    float Right[3] = { 1.0f, 0.0f, 0.0f };
    float Up[3] = { 0.0f, 1.0f, 0.0f };
    float Front[3] = { 0.0f, 0.0f, 1.0f };
    vtkMTimeType ModifiedTime = 0;
  };

  bool Update(vtkWebGPUConfiguration* configuration, const Source& source);

  bool IsReady() const { return this->Ready; }

  WGPUSampler GetSampler() const { return this->Sampler; }
  WGPUTextureView GetPrefilterView() const { return this->PrefilterCubeView; }
  WGPUTextureView GetIrradianceView() const { return this->IrradianceCubeView; }
  WGPUTextureView GetLookupTableView() const { return this->LookupTableView; }
  WGPUBuffer GetHarmonicsBuffer() const { return this->HarmonicsBuffer; }
  static constexpr std::uint64_t GetHarmonicsBufferSize() { return 9 * 4 * sizeof(float); }

  std::uint32_t GetPrefilterLevels() const { return this->PrefilterLevels; }

  /**
   * Reads the generated textures back and writes them in the given directory: the lookup table,
   * every face of the irradiance cube and of each prefilter level as PFM images, and the spherical
   * harmonics coefficients as text. Returns false when a file could not be written.
   */
  bool Dump(vtkWebGPUConfiguration* configuration, const std::string& directory);

  void ReleaseGraphicsResources();

private:
  bool CreatePipelines(vtkWebGPUConfiguration* configuration);
  void CreateTextures(vtkWebGPUConfiguration* configuration, std::uint32_t size);
  void ComputeLookupTable(vtkWebGPUConfiguration* configuration, WGPUCommandEncoder encoder);
  void ComputeEnvironment(
    vtkWebGPUConfiguration* configuration, WGPUCommandEncoder encoder, const Source& source);

  bool Ready = false;
  vtkMTimeType SourceTime = 0;
  bool SourceConvertToLinear = false;
  bool SourceIsCubeMap = false;
  WGPUTextureView SourceView = nullptr;
  float SourceBasis[9] = {};

  std::uint32_t EnvironmentSize = 0;
  std::uint32_t EnvironmentLevels = 0;
  std::uint32_t PrefilterLevels = 5;

  vtkWebGPU::Sampler Sampler;
  vtkWebGPU::Sampler SourceSampler;
  vtkWebGPU::Texture EnvironmentCube;
  vtkWebGPU::TextureView EnvironmentCubeView;
  vtkWebGPU::Texture PrefilterCube;
  vtkWebGPU::TextureView PrefilterCubeView;
  vtkWebGPU::Texture IrradianceCube;
  vtkWebGPU::TextureView IrradianceCubeView;
  vtkWebGPU::Texture LookupTable;
  vtkWebGPU::TextureView LookupTableView;
  vtkWebGPU::Buffer HarmonicsBuffer;

  vtkWebGPU::BindGroupLayout EnvironmentLayout;
  vtkWebGPU::ComputePipeline EquirectangularPipeline;
  vtkWebGPU::BindGroupLayout CubeLayout;
  vtkWebGPU::ComputePipeline CubePipeline;
  vtkWebGPU::BindGroupLayout DownsampleLayout;
  vtkWebGPU::ComputePipeline DownsamplePipeline;
  vtkWebGPU::BindGroupLayout PrefilterLayout;
  vtkWebGPU::ComputePipeline PrefilterPipeline;
  vtkWebGPU::BindGroupLayout IrradianceLayout;
  vtkWebGPU::ComputePipeline IrradiancePipeline;
  vtkWebGPU::BindGroupLayout HarmonicsLayout;
  vtkWebGPU::ComputePipeline HarmonicsPipeline;
  vtkWebGPU::BindGroupLayout LookupTableLayout;
  vtkWebGPU::ComputePipeline LookupTablePipeline;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkWebGPUEnvironmentMap.h
