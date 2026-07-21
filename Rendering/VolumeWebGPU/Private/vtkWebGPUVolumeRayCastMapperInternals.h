// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUVolumeRayCastMapperInternals
 * @brief   Device side state of vtkWebGPUVolumeRayCastMapper
 *
 * Everything that names a WebGPU type lives here so that the public header of the mapper stays
 * free of webgpu_cpp.h and of the C++ handle wrappers it brings with it.
 */

#ifndef vtkWebGPUVolumeRayCastMapperInternals_h
#define vtkWebGPUVolumeRayCastMapperInternals_h

#include "vtkWebGPUVolumeRayCastMapper.h"

#include "vtkSmartPointer.h"
#include "vtk_wgpu.h"

#include <array>
#include <cstdint>
#include <map>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkImageData;
class vtkMatrix4x4;
class vtkRenderer;
class vtkVolume;
class vtkVolumeProperty;
class vtkWebGPURenderWindow;

class vtkWebGPUVolumeRayCastMapper::vtkInternals
{
public:
  static constexpr int MaximumVolumes = vtkWebGPUVolumeRayCastMapper::MaximumNumberOfVolumes;
  static constexpr int MaximumLights = 6;
  static constexpr int MaximumClipPlanes = 6;
  static constexpr int TransferFunctionSize = 256;

  explicit vtkInternals(vtkWebGPUVolumeRayCastMapper* self)
    : Self(self)
  {
  }

  /**
   * Upload whatever the device needs that does not depend on the camera or on the depth left
   * behind by the opaque geometry. Called in the SyncDeviceResources stage.
   */
  void SyncDeviceResources(vtkWebGPURenderWindow* window, vtkVolume* volume);

  /**
   * Update the camera dependent state and record the ray casting pass. Called after the main
   * rasterization pass has been flushed, so the depth buffer is complete and free to sample.
   */
  void Render(vtkWebGPURenderWindow* window, vtkRenderer* renderer, vtkVolume* volume);

  void ReleaseGraphicsResources();

  /// Per port appearance overrides, used when the inputs are not wrapped in a vtkMultiVolume.
  std::map<int, vtkSmartPointer<vtkVolumeProperty>> PortProperties;

private:
  struct Input
  {
    int Port = 0;
    vtkImageData* Data = nullptr;
    vtkVolumeProperty* Property = nullptr;
    vtkMatrix4x4* WorldMatrix = nullptr;

    wgpu::Texture Texture;
    wgpu::TextureView TextureView;

    int Dimensions[3] = { 0, 0, 0 };
    double Spacing[3] = { 1.0, 1.0, 1.0 };
    float ValueScale = 1.0f;
    float ValueBias = 0.0f;

    double DataRange[2] = { 0.0, 1.0 };
    /// Range the color and opacity functions are tabulated over.
    double ScalarRange[2] = { 0.0, 1.0 };
    /// Range the gradient opacity function is tabulated over.
    double GradientRange[2] = { 0.0, 1.0 };

    /// Scalar extremes of each block, in raw data units.
    int BrickDimensions[3] = { 0, 0, 0 };
    int BrickSize = 0;
    std::vector<float> BrickMinimum;
    std::vector<float> BrickMaximum;
    std::uint32_t BrickOffset = 0;

    vtkMTimeType DataTime = 0;
  };

  bool CollectInputs(vtkVolume* volume);
  void UpdateVolumeTexture(vtkWebGPURenderWindow* window, Input& input);
  void UpdateTransferFunctions(vtkWebGPURenderWindow* window);
  void UpdateBrickClassification(vtkWebGPURenderWindow* window);
  void UpdateDepthView(vtkWebGPURenderWindow* window);
  void UpdateUniforms(vtkWebGPURenderWindow* window, vtkRenderer* renderer, vtkVolume* volume);
  void UpdateBindGroup(vtkWebGPURenderWindow* window);
  void EnsurePipeline(vtkWebGPURenderWindow* window);
  void EnsurePlaceholders(vtkWebGPURenderWindow* window);
  void RecordRenderPass(vtkWebGPURenderWindow* window);

  vtkWebGPUVolumeRayCastMapper* Self = nullptr;

  /// Device state of every port that has ever been rendered, keyed by port.
  std::map<int, Input> InputCache;
  /// Ports rendered this frame. The position in this list is the index the shader uses.
  std::vector<int> ActivePorts;

  wgpu::Buffer GlobalUniformBuffer;
  wgpu::Buffer VolumeParamsBuffer;
  wgpu::Buffer BrickBuffer;
  std::size_t BrickBufferElements = 0;

  wgpu::Texture ColorTransferFunctions;
  wgpu::TextureView ColorTransferFunctionsView;
  wgpu::Texture OpacityTransferFunctions;
  wgpu::TextureView OpacityTransferFunctionsView;
  wgpu::Texture GradientTransferFunctions;
  wgpu::TextureView GradientTransferFunctionsView;
  wgpu::Sampler TransferFunctionSampler;

  wgpu::Texture PlaceholderVolume;
  wgpu::TextureView PlaceholderVolumeView;
  /// Depth left behind by the opaque geometry, rebuilt every frame because the attachment is
  /// recreated whenever the window is resized.
  wgpu::TextureView OpaqueDepthView;

  wgpu::BindGroupLayout BindGroupLayout;
  wgpu::BindGroup BindGroup;
  wgpu::RenderPipeline Pipeline;
  wgpu::TextureFormat PipelineColorFormat = wgpu::TextureFormat::Undefined;

  /// Opacity table of every input, kept so blocks can be reclassified without asking the
  /// transfer functions again.
  std::vector<std::array<float, TransferFunctionSize>> OpacityTables;
  vtkMTimeType TransferFunctionTime = 0;
  vtkMTimeType BrickClassificationTime = 0;

  /// Whether the depth of the opaque geometry can be sampled this frame.
  bool DepthAvailable = false;

  /// Renderer viewport in framebuffer pixels: origin x, origin y, width, height.
  float ViewportRectangle[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
  bool DrawFullScreen = false;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkWebGPUVolumeRayCastMapperInternals.h
