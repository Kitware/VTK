// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUContextActor
 * @brief   provides a vtkProp derived object.
 *
 * This object provides the entry point for the vtkContextScene to be rendered
 * in a vtkRenderer. Uses the RenderOverlay pass to render the 2D
 * vtkContextScene.
 */

#ifndef vtkWebGPUContextActor_h
#define vtkWebGPUContextActor_h

#include "vtkContextActor.h"
#include "vtkRenderingWebGPUModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUContextActor : public vtkContextActor
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkWebGPUContextActor, vtkContextActor);

  static vtkWebGPUContextActor* New();

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow* window) override;

  /**
   * We only render in the overlay for the context scene.
   */
  int RenderOverlay(vtkViewport* viewport) override;

protected:
  vtkWebGPUContextActor();
  ~vtkWebGPUContextActor() override;

  /**
   * Initialize the actor - right now we just decide which device to initialize.
   */
  void Initialize(vtkViewport* /*viewport*/) override;

private:
  vtkWebGPUContextActor(const vtkWebGPUContextActor&) = delete;
  void operator=(const vtkWebGPUContextActor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
