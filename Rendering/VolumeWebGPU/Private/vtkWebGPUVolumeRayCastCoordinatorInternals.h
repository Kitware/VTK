// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUVolumeRayCastCoordinatorInternals
 * @brief   Shared device state of vtkWebGPUVolumeRayCastCoordinator
 *
 * Everything that is shared by the volumes of one pass lives here: the transfer function arrays,
 * the block buffer, the parameter buffers, the bind group and the pipeline. The per input
 * resources stay with the mapper that owns them and arrive as contributions.
 */

#ifndef vtkWebGPUVolumeRayCastCoordinatorInternals_h
#define vtkWebGPUVolumeRayCastCoordinatorInternals_h

#include "vtkWebGPUVolumeRayCastCoordinator.h"

#include "Private/vtkWebGPUVolumeRayCastContribution.h"

#include "vtk_wgpu.h"

#include <cstdint>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkRenderer;
class vtkWebGPURenderWindow;

class vtkWebGPUVolumeRayCastCoordinator::vtkInternals
{
public:
  static constexpr int MaximumVolumes = vtkWebGPUVolumeRayCastCoordinator::MaximumNumberOfVolumes;
  static constexpr int MaximumLights = 6;
  static constexpr int MaximumClipPlanes = 6;
  static constexpr int TableSize = vtkWebGPUVolumeTransferFunctionSize;

  explicit vtkInternals(vtkWebGPUVolumeRayCastCoordinator* self)
    : Self(self)
  {
  }

  /**
   * Take one input. Starts a new frame when the previous one was drawn, or when the same input
   * turns up twice, meaning a frame was gathered but never drawn. Returns false once full.
   */
  bool Offer(const vtkWebGPUVolumeRayCastContribution& contribution);

  /// March every gathered input and composite the result over the frame, once.
  void Render(vtkWebGPURenderWindow* window, vtkRenderer* renderer);

  void ReleaseGraphicsResources();
  /// Forget what a mapper contributed, because it is letting go of its resources.
  void Forget(vtkWebGPUVolumeRayCastMapper* mapper);

  std::size_t GetNumberOfContributions() const { return this->Contributions.size(); }

  /// Whether the pass for this frame has already been recorded.
  bool Rendered = true;

private:
  void BeginFrame();
  /// Drop everything built on the device, keeping what was gathered for this frame.
  void ReleaseDeviceResources();

  void UpdateTransferFunctions(vtkWebGPURenderWindow* window);
  void UpdateBricks(vtkWebGPURenderWindow* window);
  void UpdateDepthView(vtkWebGPURenderWindow* window);
  void UpdateUniforms(vtkWebGPURenderWindow* window, vtkRenderer* renderer);
  void UpdateBindGroup(vtkWebGPURenderWindow* window);
  void EnsurePipeline(vtkWebGPURenderWindow* window);
  void EnsurePlaceholders(vtkWebGPURenderWindow* window);
  void RecordRenderPass(vtkWebGPURenderWindow* window);

  vtkWebGPUVolumeRayCastCoordinator* Self = nullptr;

  std::vector<vtkWebGPUVolumeRayCastContribution> Contributions;

  /// What each layer of the transfer function arrays currently holds, so unchanged inputs are
  /// not uploaded again.
  struct LayerState
  {
    vtkWebGPUVolumeRayCastMapper* Mapper = nullptr;
    int Port = -1;
    vtkMTimeType ContentTime = 0;
  };
  std::vector<LayerState> Layers;
  std::vector<LayerState> BrickLayers;
  /// Where the blocks of each contribution start in the shared buffer.
  std::vector<std::uint32_t> BrickOffsets;

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

  /// The device everything below was built on, so a renderer handed to another render window
  /// rebuilds rather than reaching across devices. Not a reference: the render window tears its
  /// device down in a particular order and must hold the last one.
  WGPUDevice DeviceIdentity = nullptr;

  wgpu::BindGroupLayout BindGroupLayout;
  wgpu::BindGroup BindGroup;
  wgpu::RenderPipeline Pipeline;
  wgpu::TextureFormat PipelineColorFormat = wgpu::TextureFormat::Undefined;

  bool DepthAvailable = false;
  /// Renderer viewport in framebuffer pixels: origin x, origin y, width, height.
  float ViewportRectangle[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
  bool DrawFullScreen = false;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkWebGPUVolumeRayCastCoordinatorInternals.h
