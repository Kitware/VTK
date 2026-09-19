// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUVolumeRayCastMapper
 * @brief   GPU ray cast volume mapper for the WebGPU backend
 *
 * vtkWebGPUVolumeRayCastMapper renders vtkImageData volumes by marching rays
 * through 3D textures in a WGSL fragment shader. It is the WebGPU counterpart
 * of vtkOpenGLGPUVolumeRayCastMapper and is installed as an object factory
 * override of vtkGPUVolumeRayCastMapper.
 *
 * Two aspects differ from the OpenGL implementation:
 *
 * - Volumes that share a renderer are ray cast together, in one pass, and are
 *   allowed to overlap in space. Adding several vtkVolume props to a renderer
 *   is all it takes, each with its own mapper and its own transfer functions;
 *   neither a vtkMultiVolume nor several inputs on one mapper is needed,
 *   though both still work. Overlapping samples are blended as co-located
 *   participating media, so the result does not depend on the order the props
 *   were added in. See vtkWebGPUVolumeRayCastCoordinator, which is where the
 *   volumes of a renderer are brought back together.
 *
 *   A single mapper may still carry several inputs on consecutive ports, each
 *   with its own vtkVolumeProperty set through SetVolumeProperty(); those are
 *   gathered into the same pass as everything else.
 *
 * - The distance between samples is not fixed along a ray. The mapper
 *   maintains a coarse min/max acceleration structure over each input and uses
 *   it, together with the current transfer functions, to skip regions that the
 *   transfer function maps to nothing and to sample regions that carry detail
 *   more densely than regions that do not. See SetAdaptiveSampling().
 *
 * @sa
 * vtkWebGPUVolumeRayCastCoordinator vtkGPUVolumeRayCastMapper
 * vtkOpenGLGPUVolumeRayCastMapper
 */

#ifndef vtkWebGPUVolumeRayCastMapper_h
#define vtkWebGPUVolumeRayCastMapper_h

#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkRenderingVolumeWebGPUModule.h" // for export macro
#include "vtkWeakPointer.h"                 // for vtkWeakPointer

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class vtkOverrideAttribute;
class vtkVolumeProperty;

class VTKRENDERINGVOLUMEWEBGPU_EXPORT vtkWebGPUVolumeRayCastMapper
  : public vtkGPUVolumeRayCastMapper
{
public:
  static vtkWebGPUVolumeRayCastMapper* New();
  vtkTypeMacro(vtkWebGPUVolumeRayCastMapper, vtkGPUVolumeRayCastMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkOverrideAttribute* CreateOverrideAttributes();

  /**
   * Bounds of every connected input taken together, so that a mapper with several inputs is
   * framed and culled as a whole even when it is not held by a vtkMultiVolume.
   */
  double* GetBounds() VTK_SIZEHINT(6) override;
  using Superclass::GetBounds;

  void GPURender(vtkRenderer* ren, vtkVolume* vol) override;

  /**
   * Reads streamed inputs a brick at a time instead of updating them whole first.
   */
  void Render(vtkRenderer* ren, vtkVolume* vol) override;

  /**
   * Asks nothing of a streamed input when the mapper is updated, as vtkVolume does before every
   * render: its bricks are read as they are needed instead.
   */
  vtkTypeBool ProcessRequest(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  ///@{
  /**
   * What the last render produced while RenderToImage was on: the volume drawn alone over a
   * transparent background, and the window space depth of the first sample each ray found visible.
   * Both are empty until such a render has happened.
   */
  void GetColorImage(vtkImageData* output) override;
  void GetDepthImage(vtkImageData* output) override;
  ///@}
  int IsRenderSupported(vtkRenderWindow* window, vtkVolumeProperty* property) override;
  void ReleaseGraphicsResources(vtkWindow* window) override;

  /**
   * Let go of what this mapper contributed to the pass of \a renderer as well as of its own
   * resources.
   */
  void ReleaseGraphicsResources(vtkRenderer* renderer);
  void GetReductionRatio(double ratio[3]) override;

  ///@{
  /**
   * Appearance of the input connected to \a port. Without one, the property of the vtkVolume
   * being rendered is used, which gives each of several overlapping inputs its own transfer
   * functions without a vtkMultiVolume.
   */
  void SetVolumeProperty(int port, vtkVolumeProperty* property);
  vtkVolumeProperty* GetVolumeProperty(int port);
  ///@}

  ///@{
  /**
   * Vary the step along a ray with how much detail the volume carries there, and skip regions the
   * transfer functions map to nothing. On by default.
   *
   * The step is chosen per sample between MinimumSampleDistanceFactor and
   * MaximumSampleDistanceFactor times the base sample distance. Off marches at the base distance
   * everywhere, as vtkOpenGLGPUVolumeRayCastMapper does.
   */
  vtkSetMacro(AdaptiveSampling, bool);
  vtkGetMacro(AdaptiveSampling, bool);
  vtkBooleanMacro(AdaptiveSampling, bool);
  ///@}

  ///@{
  /**
   * Jump to the far side of a block whose contents the transfer functions map to an opacity no
   * larger than EmptySpaceOpacityThreshold. Requires AdaptiveSampling. On by default.
   */
  vtkSetMacro(EmptySpaceSkipping, bool);
  vtkGetMacro(EmptySpaceSkipping, bool);
  vtkBooleanMacro(EmptySpaceSkipping, bool);
  ///@}

  ///@{
  /**
   * Edge length, in voxels, of the bricks the volume is cut into. Bricks are both what the
   * acceleration structure classifies and what is uploaded, so smaller ones skip empty space and
   * stream more precisely while larger ones spend less on each brick's one voxel apron and keep
   * the page table smaller. Defaults to 32.
   *
   * Every volume drawn by one renderer shares a pool and so this value; the first mapper to
   * prepare its inputs settles it.
   */
  vtkSetClampMacro(BrickSize, int, 4, 128);
  vtkGetMacro(BrickSize, int);
  ///@}

  ///@{
  /**
   * Bounds, as multiples of the base sample distance, of the step adaptive sampling may choose.
   * The minimum is used where the volume carries the most detail and the maximum where it carries
   * the least. Default to 1 and 6.
   */
  vtkSetClampMacro(MinimumSampleDistanceFactor, double, 0.05, 100.0);
  vtkGetMacro(MinimumSampleDistanceFactor, double);
  vtkSetClampMacro(MaximumSampleDistanceFactor, double, 0.05, 100.0);
  vtkGetMacro(MaximumSampleDistanceFactor, double);
  ///@}

  ///@{
  /**
   * Largest opacity a block may reach before it stops counting as empty.
   * Defaults to 0.001.
   */
  vtkSetClampMacro(EmptySpaceOpacityThreshold, double, 0.0, 1.0);
  vtkGetMacro(EmptySpaceOpacityThreshold, double);
  ///@}

  ///@{
  /**
   * Ceiling on the samples a ray may take while the scene is being interacted with, that is while
   * the volume has less than a second of render time. Only consulted when
   * AutoAdjustSampleDistances is on. Defaults to 256.
   */
  vtkSetClampMacro(InteractiveMaximumNumberOfSamples, int, 16, 65536);
  vtkGetMacro(InteractiveMaximumNumberOfSamples, int);
  ///@}

  ///@{
  /**
   * Read an input through the pipeline a brick at a time, rather than updating it whole, when its
   * source can produce sub-extents and the volume would otherwise take more than this many
   * megabytes. Only the bricks the rays reach are read, and the volume fills in over the frames
   * that follow; vtkWebGPUVolumeRayCastCoordinator::GetComplete() says when it has. 0 streams
   * whenever the source allows it. Defaults to 1024.
   *
   * The transfer functions of a streamed input are mapped over the range its source publishes
   * with vtkDataObject::FIELD_RANGE(), or over their own range when none is published. An input
   * with a mask, a second transfer function axis array, cell scalars or ghost arrays is read whole.
   */
  vtkSetClampMacro(StreamingThresholdMB, int, 0, VTK_INT_MAX);
  vtkGetMacro(StreamingThresholdMB, int);
  ///@}

  /**
   * Whether the input on \a port is read a brick at a time.
   */
  bool IsStreaming(int port = 0);

  /// Device side state, defined in a private header so that no WebGPU type reaches this one.
  class vtkInternals;

protected:
  vtkWebGPUVolumeRayCastMapper();
  ~vtkWebGPUVolumeRayCastMapper() override;

  // AMR rendering is not supported by this mapper.
  void PreRender(vtkRenderer* ren, vtkVolume* vol, double datasetBounds[6], double scalarRange[2],
    int numberOfScalarComponents, unsigned int numberOfLevels) override;
  void RenderBlock(vtkRenderer* ren, vtkVolume* vol, unsigned int level) override;
  void PostRender(vtkRenderer* ren, int numberOfScalarComponents) override;

  bool AdaptiveSampling = true;
  bool EmptySpaceSkipping = true;
  int BrickSize = 8;
  double MinimumSampleDistanceFactor = 1.0;
  double MaximumSampleDistanceFactor = 6.0;
  double EmptySpaceOpacityThreshold = 0.001;
  int InteractiveMaximumNumberOfSamples = 256;
  int StreamingThresholdMB = 1024;

private:
  vtkWebGPUVolumeRayCastMapper(const vtkWebGPUVolumeRayCastMapper&) = delete;
  void operator=(const vtkWebGPUVolumeRayCastMapper&) = delete;

  std::unique_ptr<vtkInternals> Internals;

  /// The renderer the last GPURender ran with, so that the render to image results can be found
  /// again afterwards. Weak: the renderer owns the props, not the other way round.
  vtkWeakPointer<vtkRenderer> LastRenderer;
};

#define vtkWebGPUVolumeRayCastMapper_OVERRIDE_ATTRIBUTES                                           \
  vtkWebGPUVolumeRayCastMapper::CreateOverrideAttributes()

VTK_ABI_NAMESPACE_END
#endif
