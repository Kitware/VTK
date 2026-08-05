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
   * warning; the limit comes from the number of sampled textures a WebGPU bind group may hold for
   * one shader stage.
   */
  static constexpr int MaximumNumberOfVolumes = 12;

  ///@{
  /**
   * Called by vtkWebGPUVolumeRayCastMapper as it moves through the stages of a frame. Not meant
   * to be called from anywhere else.
   */
  void Render(vtkRenderer* renderer);
  void ReleaseGraphicsResources(vtkWebGPUVolumeRayCastMapper* mapper);
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

private:
  vtkWebGPUVolumeRayCastCoordinator(const vtkWebGPUVolumeRayCastCoordinator&) = delete;
  void operator=(const vtkWebGPUVolumeRayCastCoordinator&) = delete;

  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
