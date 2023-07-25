// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUProperty
 * @brief   WebGPU property
 *
 * vtkWebGPUProperty is a concrete implementation of the abstract class
 * vtkProperty. vtkWebGPUProperty interfaces to the WebGPU rendering library.
 */

#ifndef vtkWebGPUProperty_h
#define vtkWebGPUProperty_h

#include "vtkProperty.h"

#include "vtkRenderingWebGPUModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUProperty : public vtkProperty
{
public:
  static vtkWebGPUProperty* New();
  vtkTypeMacro(vtkWebGPUProperty, vtkProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Implement base class method.
   */
  void Render(vtkActor* a, vtkRenderer* ren) override;

  /**
   * Implement base class method.
   */
  void BackfaceRender(vtkActor* a, vtkRenderer* ren) override;

  /**
   * This method is called after the actor has been rendered.
   * Don't call this directly. This method cleans up
   * any shaders allocated.
   */
  void PostRender(vtkActor* a, vtkRenderer* r) override;

  /**
   * Release any graphics resources that are being consumed by this
   * property. The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow* win) override;

protected:
  vtkWebGPUProperty();
  ~vtkWebGPUProperty() override;

  /**
   * Method called in vtkWebGPUProperty::Render() to render textures.
   * Last argument is the value returned from RenderShaders() call.
   */
  bool RenderTextures(vtkActor* actor, vtkRenderer* renderer);

private:
  vtkWebGPUProperty(const vtkWebGPUProperty&) = delete;
  void operator=(const vtkWebGPUProperty&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
