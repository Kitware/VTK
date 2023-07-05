// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOSPRayVolumeMapper
 * @brief   Standalone OSPRayVolumeMapper.
 *
 * This is a standalone interface for ospray volume rendering to be used
 * within otherwise OpenGL rendering contexts such as within the
 * SmartVolumeMapper.
 */

#ifndef vtkOSPRayVolumeMapper_h
#define vtkOSPRayVolumeMapper_h

#include "vtkOSPRayVolumeInterface.h"
#include "vtkRenderingRayTracingModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkOSPRayPass;
class vtkRenderer;
class vtkWindow;

class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayVolumeMapper : public vtkOSPRayVolumeInterface
{
public:
  static vtkOSPRayVolumeMapper* New();
  vtkTypeMacro(vtkOSPRayVolumeMapper, vtkOSPRayVolumeInterface);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  // Initialize internal constructs
  virtual void Init();

  /**
   * Render the volume onto the screen.
   * Overridden to use OSPRay to do the work.
   */
  void Render(vtkRenderer*, vtkVolume*) override;

protected:
  vtkOSPRayVolumeMapper();
  ~vtkOSPRayVolumeMapper() override;

  vtkOSPRayPass* InternalOSPRayPass;
  vtkRenderer* InternalRenderer;
  bool Initialized;

private:
  vtkOSPRayVolumeMapper(const vtkOSPRayVolumeMapper&) = delete;
  void operator=(const vtkOSPRayVolumeMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
