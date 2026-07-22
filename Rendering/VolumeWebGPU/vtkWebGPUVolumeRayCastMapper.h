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
 * - Any number of inputs (up to MaximumNumberOfVolumes) are ray cast together
 *   in a single pass, and the inputs are allowed to overlap in space. A
 *   secondary vtkMultiVolume prop is not required; connect the inputs to
 *   consecutive ports and, optionally, give each port its own
 *   vtkVolumeProperty with SetVolumeProperty(). A vtkMultiVolume is still
 *   accepted for source compatibility. Overlapping samples are blended as
 *   co-located participating media, so the result is independent of the order
 *   in which the inputs were connected.
 *
 * - The distance between samples is not fixed along a ray. The mapper
 *   maintains a coarse min/max acceleration structure over each input and uses
 *   it, together with the current transfer functions, to skip regions that the
 *   transfer function maps to nothing and to sample regions that carry detail
 *   more densely than regions that do not. See SetAdaptiveSampling().
 *
 * @sa
 * vtkGPUVolumeRayCastMapper vtkOpenGLGPUVolumeRayCastMapper
 */

#ifndef vtkWebGPUVolumeRayCastMapper_h
#define vtkWebGPUVolumeRayCastMapper_h

#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkRenderingVolumeWebGPUModule.h" // for export macro

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN

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
   * The largest number of inputs that can be ray cast in a single pass. The
   * limit comes from the number of sampled textures a WebGPU bind group may
   * hold for one shader stage.
   */
  static constexpr int MaximumNumberOfVolumes = 12;

  void GPURender(vtkRenderer* ren, vtkVolume* vol) override;
  int IsRenderSupported(vtkRenderWindow* window, vtkVolumeProperty* property) override;
  void ReleaseGraphicsResources(vtkWindow* window) override;
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
   * Edge length, in voxels, of the blocks the acceleration structure is built
   * over. Smaller blocks classify the volume more precisely but cost more
   * memory and take longer to reclassify when a transfer function changes.
   * Defaults to 8.
   */
  vtkSetClampMacro(BrickSize, int, 2, 64);
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
   * Offset the start of each ray by a pseudo-random fraction of a step, which
   * trades the banding that regular sampling produces for noise. On by
   * default.
   */
  vtkSetMacro(Jittering, bool);
  vtkGetMacro(Jittering, bool);
  vtkBooleanMacro(Jittering, bool);
  ///@}

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
  bool Jittering = true;
  int BrickSize = 8;
  double MinimumSampleDistanceFactor = 1.0;
  double MaximumSampleDistanceFactor = 6.0;
  double EmptySpaceOpacityThreshold = 0.001;
  int InteractiveMaximumNumberOfSamples = 256;

private:
  vtkWebGPUVolumeRayCastMapper(const vtkWebGPUVolumeRayCastMapper&) = delete;
  void operator=(const vtkWebGPUVolumeRayCastMapper&) = delete;

  std::unique_ptr<vtkInternals> Internals;
};

#define vtkWebGPUVolumeRayCastMapper_OVERRIDE_ATTRIBUTES                                           \
  vtkWebGPUVolumeRayCastMapper::CreateOverrideAttributes()

VTK_ABI_NAMESPACE_END
#endif
