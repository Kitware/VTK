// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLProperty
 * @brief   OpenGL property
 *
 * vtkOpenGLProperty is a concrete implementation of the abstract class
 * vtkProperty. vtkOpenGLProperty interfaces to the OpenGL rendering library.
 */

#ifndef vtkOpenGLProperty_h
#define vtkOpenGLProperty_h

#include "vtkProperty.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLProperty : public vtkProperty
{
public:
  static vtkOpenGLProperty* New();
  vtkTypeMacro(vtkOpenGLProperty, vtkProperty);
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
  vtkOpenGLProperty();
  ~vtkOpenGLProperty() override;

  /**
   * Method called in vtkOpenGLProperty::Render() to render textures.
   * Last argument is the value returned from RenderShaders() call.
   */
  bool RenderTextures(vtkActor* actor, vtkRenderer* renderer);

private:
  vtkOpenGLProperty(const vtkOpenGLProperty&) = delete;
  void operator=(const vtkOpenGLProperty&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
