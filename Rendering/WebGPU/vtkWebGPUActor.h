// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUActor_h
#define vtkWebGPUActor_h

#include "vtkActor.h"

#include "vtkProperty.h"              // for VTK_FLAT
#include "vtkRenderingWebGPUModule.h" // for export macro

#include <memory> // for unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkMatrix3x3;
class vtkWebGPUConfiguration;
class vtkWebGPURenderPipelineCache;
class vtkWebGPUActorInternals;
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUActor : public vtkActor
{
public:
  static vtkWebGPUActor* New();
  vtkTypeMacro(vtkWebGPUActor, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void ReleaseGraphicsResources(vtkWindow* window) override;

  void ShallowCopy(vtkProp* other) override;

  /**
   * Actual actor render method.
   */
  void Render(vtkRenderer* renderer, vtkMapper* mapper) override;

  ///@{
  /**
   * Does this prop have opaque/translucent polygonal geometry?
   * These methods are overridden to skip redundant checks
   * in different rendering stages.
   *
   * If the mapper has already been checked for opaque geometry and the mapper
   * has not been modified since the last check, this method uses the last result,
   * instead of asking the mapper to check for opaque geometry again. The
   * HasTranslucentPolygonalGeometry() similarly checks and caches the result of
   * vtkMapper::HasTranslucentPolygonalGeometry()
   *
   * @sa vtkWebGPURenderer::GetRenderStage()
   */
  vtkTypeBool HasOpaqueGeometry() override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

protected:
  vtkWebGPUActor();
  ~vtkWebGPUActor() override;

private:
  // for accessing SupportsRenderBundles
  friend class vtkWebGPURenderer;
  // for accessing vtkWebGPUActorInternals::PopulateBindGroupLayouts
  friend class vtkWebGPUPolyDataMapper;
  friend class vtkWebGPUGlyph3DMapperHelper;

  vtkWebGPUActor(const vtkWebGPUActor&) = delete;
  void operator=(const vtkWebGPUActor&) = delete;

  std::unique_ptr<vtkWebGPUActorInternals> Internals;

  /**
   * Returns true if the actor supports rendering with render bundles, false otherwise.
   *
   * This is mainly used for the point cloud mapper. This mapper doesn't use the rasterization
   * pipeline for the rendering and thus doesn't support render bundles.
   */
  bool SupportRenderBundles();
  void SetId(vtkTypeUInt32 id);

  bool UpdateKeyMatrices();

  const void* GetCachedActorInformation();
  static std::size_t GetCacheSizeBytes();

  bool CacheActorTransforms();
  bool CacheActorRenderOptions();
  bool CacheActorShadeOptions();
  bool CacheActorId();

  void AllocateResources(vtkWebGPUConfiguration* renderer);
};

VTK_ABI_NAMESPACE_END
#endif
