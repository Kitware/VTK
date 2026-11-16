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

#include "Private/vtkWebGPUHandle.h" // for the reference counted handle wrappers
#include "Private/vtkWebGPUVolumeBrickPool.h"
#include "Private/vtkWebGPUVolumeRayCastContribution.h"

#include "vtk_wgpu.h"

#include <array>
#include <cstdint>
#include <map>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkRenderer;
class vtkWebGPURenderWindow;

class vtkWebGPUVolumeRayCastCoordinator::vtkInternals
{
public:
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

  /// Whether every brick the last frame wanted was there.
  bool WasComplete() const { return this->Complete; }

  /// Whether the pass for this frame has already been recorded.
  bool Rendered = true;

  /// Render to image draws the volume alone into textures of this object's own, with the depth of
  /// the first sample that was visible where the selection ids otherwise go. The window's
  /// attachments are left alone, which is what makes the mode ignore the rest of the scene.
  bool RenderToImage = false;
  /// The window the last pass was drawn with, so the image can be read back afterwards. Not a
  /// reference: the render window outlives the coordinator it owns.
  vtkWebGPURenderWindow* LastWindow = nullptr;
  vtkWebGPU::Texture ImageColor;
  vtkWebGPU::TextureView ImageColorView;
  vtkWebGPU::Texture ImageDepth;
  vtkWebGPU::TextureView ImageDepthView;
  int ImageSize[2] = { 0, 0 };
  void EnsureImageTargets(vtkWebGPURenderWindow* window);
  /// Copy one of the two image targets back into \a output. Returns false when nothing has been
  /// drawn into them yet.
  bool ReadImage(vtkWebGPURenderWindow* window, bool depth, int scalarType, vtkImageData* output);

private:
  void BeginFrame();
  /// Drop everything built on the device, keeping what was gathered for this frame.
  void ReleaseDeviceResources();

  void UpdateTransferFunctions(vtkWebGPURenderWindow* window);
  void UpdateCells(vtkWebGPURenderWindow* window);
  void UpdateResidency(vtkWebGPURenderWindow* window, vtkRenderer* renderer);
  void UpdateDepthView(vtkWebGPURenderWindow* window);
  void UpdateUniforms(vtkWebGPURenderWindow* window, vtkRenderer* renderer);
  void UpdateBindGroup(vtkWebGPURenderWindow* window);
  void EnsurePipeline(vtkWebGPURenderWindow* window);
  void EnsurePlaceholders(vtkWebGPURenderWindow* window);
  void RecordRenderPass(vtkWebGPURenderWindow* window);

  /// Check if volumes can potentially overlap based on their bounding boxes.
  bool VolumesCanOverlap() const;
  /// Check if all volumes have the same blend mode.
  bool AllBlendModesMatch(int& outMode) const;

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
  std::vector<LayerState> CellLayers;
  /// What each volume held when its bricks were last uploaded, so that a volume whose data has
  /// been replaced does not go on being drawn from what the pool still holds.
  std::map<const void*, vtkMTimeType> BrickContent;
  /// Where the pages and the acceleration cells of each contribution start in the shared tables.
  std::vector<std::uint32_t> PageOffsets;
  std::vector<std::uint32_t> CellOffsets;
  /// How many bricks each contribution is cut into, at the pool's brick size.
  std::vector<std::array<int, 3>> PageDimensions;
  /// Texture space to world space for each contribution, kept from the uniform update so that
  /// residency can be decided on where the bricks actually are.
  std::vector<std::array<double, 16>> VolumeMatrices;

  /// Bricks of every volume, and the table that says where each one landed.
  vtkWebGPUVolumeBrickPool Pool;
  bool Complete = true;
  /// Whether the pass is allowed to leave bricks the transfer functions map to nothing out of
  /// the pool entirely, decided with the rest of the march in UpdateUniforms.
  bool SkipEmptyBricks = false;
  float EmptyThreshold = 0.0f;

  vtkWebGPU::Buffer GlobalUniformBuffer;
  vtkWebGPU::Buffer VolumeParamsBuffer;
  vtkWebGPU::Buffer PageBuffer;
  std::size_t PageBufferElements = 0;
  /// What the device already holds, so an unchanged table is not sent again.
  std::vector<unsigned char> PageTable;

  /// Two floats per acceleration cell of every volume: the largest opacity the transfer
  /// functions map it to, and how much it varies.
  vtkWebGPU::Buffer CellBuffer;
  std::size_t CellBufferElements = 0;

  vtkWebGPU::Texture ColorTransferFunctions;
  vtkWebGPU::TextureView ColorTransferFunctionsView;
  vtkWebGPU::Texture OpacityTransferFunctions;
  vtkWebGPU::TextureView OpacityTransferFunctionsView;
  vtkWebGPU::Texture GradientTransferFunctions;
  vtkWebGPU::TextureView GradientTransferFunctionsView;
  /// Two dimensional transfer functions, created only if some volume of the pass asks for one.
  vtkWebGPU::Texture Transfer2DFunctions;
  vtkWebGPU::TextureView Transfer2DFunctionsView;
  std::vector<LayerState> Transfer2DLayers;

  /// One row per label of the label map, one layer per volume. Only made when a volume of the
  /// pass is actually drawn through a label map, and remade when a longer one comes along.
  vtkWebGPU::Texture LabelTransferFunctions;
  vtkWebGPU::TextureView LabelTransferFunctionsView;
  std::vector<LayerState> LabelLayers;
  int LabelRows = 0;
  vtkWebGPU::Sampler TransferFunctionSampler;

  /// Depth left behind by the opaque geometry, rebuilt every frame because the attachment is
  /// recreated whenever the window is resized.
  vtkWebGPU::TextureView OpaqueDepthView;

  /// The device everything below was built on, so a renderer handed to another render window
  /// rebuilds rather than reaching across devices. Not a reference: the render window tears its
  /// device down in a particular order and must hold the last one.
  WGPUDevice DeviceIdentity = nullptr;

  vtkWebGPU::BindGroupLayout BindGroupLayout;
  vtkWebGPU::BindGroup BindGroup;
  vtkWebGPU::RenderPipeline Pipeline;
  WGPUTextureFormat PipelineColorFormat = WGPUTextureFormat_Undefined;
  bool PipelineRenderToImage = false;

  bool DepthAvailable = false;
  /// Renderer viewport in framebuffer pixels: origin x, origin y, width, height.
  float ViewportRectangle[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
  bool DrawFullScreen = false;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkWebGPUVolumeRayCastCoordinatorInternals.h
