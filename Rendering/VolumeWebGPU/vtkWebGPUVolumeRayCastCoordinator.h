// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUVolumeRayCastCoordinator
 * @brief   Ray casts every WebGPU volume in a renderer in one pass
 *
 * Volume rendering only composites correctly when the volumes that share a ray are sampled
 * together. Handing each vtkVolume to its own mapper and letting each mapper draw in turn cannot
 * do that: every mapper would composite a finished image over the previous one, which is wrong
 * wherever two volumes occupy the same space and is order dependent everywhere else.
 *
 * This class is where the volumes of a renderer are brought back together. Each
 * vtkWebGPUVolumeRayCastMapper prepares its own inputs, uploading textures and tabulating
 * transfer functions, and then offers them to the coordinator of the renderer it is drawing into.
 * Once the opaque geometry has been rasterized, the coordinator marches one set of rays through
 * everything it was offered and composites the result over the frame a single time.
 *
 * Nothing has to be assembled by hand for this to happen. Adding several vtkVolume props to a
 * renderer is enough, whether they overlap or not, and whether they came from one mapper with
 * several inputs or from a mapper each. A vtkMultiVolume still works but is no longer needed.
 *
 * There is one coordinator per renderer, created on demand and released with the renderer.
 *
 * @sa
 * vtkWebGPUVolumeRayCastMapper
 */

#ifndef vtkWebGPUVolumeRayCastCoordinator_h
#define vtkWebGPUVolumeRayCastCoordinator_h

#include "vtkObject.h"
#include "vtkRenderingVolumeWebGPUModule.h" // for export macro

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class vtkRenderer;
class vtkVolume;
class vtkWebGPUVolumeRayCastMapper;

class VTKRENDERINGVOLUMEWEBGPU_EXPORT vtkWebGPUVolumeRayCastCoordinator : public vtkObject
{
public:
  static vtkWebGPUVolumeRayCastCoordinator* New();
  vtkTypeMacro(vtkWebGPUVolumeRayCastCoordinator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The coordinator of \a renderer, created the first time it is asked for and released when the
   * renderer goes away. Returns nullptr for a renderer that is not a vtkWebGPURenderer.
   */
  static vtkWebGPUVolumeRayCastCoordinator* GetForRenderer(vtkRenderer* renderer);

  /**
   * Forget the coordinators of every renderer. Only useful when tearing down.
   */
  static void ReleaseAll();

  /**
   * How many inputs were gathered for the frame being drawn.
   */
  vtkGetMacro(NumberOfVolumes, int);

  /**
   * The largest number of inputs that can share a pass. Anything beyond this is dropped with a
   * warning; the limit is on how many volumes one uniform buffer describes, not on how much data
   * they hold - the bricks of every volume share one pool.
   */
  static constexpr int MaximumNumberOfVolumes = 12;

  ///@{
  /**
   * Edge length, in voxels, of the bricks every volume of this renderer is cut into. Bricks are
   * the unit of residency: what the pool holds, what it evicts and what it fetches. Larger ones
   * spend less on each brick's one voxel apron, smaller ones stream more finely.
   *
   * This is not the granularity at which empty space is found, which is the mapper's BrickSize.
   */
  vtkSetClampMacro(PoolBrickSize, int, 4, 128);
  vtkGetMacro(PoolBrickSize, int);
  ///@}

  ///@{
  /**
   * How much device memory the shared brick pool may take, in megabytes, and how many bricks may
   * be brought in per frame. Volumes larger than the pool still draw: the bricks the rays reach
   * first are kept and the rest arrive over the frames that follow.
   *
   * The default is high enough that a volume which fits is complete in one frame. Lower it to
   * spread the cost of a volume read from disk rather than stalling on any one frame.
   */
  vtkSetClampMacro(BrickPoolSizeMB, int, 16, 65536);
  vtkGetMacro(BrickPoolSizeMB, int);
  vtkSetClampMacro(MaximumBricksPerFrame, int, 1, 65536);
  vtkGetMacro(MaximumBricksPerFrame, int);
  ///@}

  /**
   * Whether the bricks the last frame asked for were all resident. False means the pool is too
   * small, or that bricks are still arriving, and that the next frame will look better.
   */
  vtkGetMacro(Complete, bool);

  ///@{
  /**
   * Called by vtkWebGPUVolumeRayCastMapper as it moves through the stages of a frame. Not meant
   * to be called from anywhere else.
   */
  void Render(vtkRenderer* renderer);
  void ReleaseGraphicsResources(vtkWebGPUVolumeRayCastMapper* mapper);
  ///@}

  ///@{
  /**
   * Copy what the last render to image produced into \a output. The colour is RGBA bytes and the
   * depth is one component of \a scalarType, holding the window space depth of the first sample
   * the ray found visible. Return false when the pass has not been drawn in that mode.
   */
  bool GetImageColor(vtkImageData* output);
  bool GetImageDepth(int scalarType, vtkImageData* output);
  ///@}

  /// Device side state, defined in a private header so that no WebGPU type reaches this one.
  class vtkInternals;
#ifndef __VTK_WRAP__
  vtkInternals* GetInternals() { return this->Internals.get(); }
#endif

protected:
  vtkWebGPUVolumeRayCastCoordinator();
  ~vtkWebGPUVolumeRayCastCoordinator() override;

  int NumberOfVolumes = 0;
  int PoolBrickSize = 32;
  int BrickPoolSizeMB = 512;
  int MaximumBricksPerFrame = 16384;
  bool Complete = true;

private:
  vtkWebGPUVolumeRayCastCoordinator(const vtkWebGPUVolumeRayCastCoordinator&) = delete;
  void operator=(const vtkWebGPUVolumeRayCastCoordinator&) = delete;

  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
