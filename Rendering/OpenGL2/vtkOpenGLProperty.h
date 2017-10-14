/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLProperty
 * @brief   OpenGL property
 *
 * vtkOpenGLProperty is a concrete implementation of the abstract class
 * vtkProperty. vtkOpenGLProperty interfaces to the OpenGL rendering library.
*/

#ifndef vtkOpenGLProperty_h
#define vtkOpenGLProperty_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkProperty.h"

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLProperty : public vtkProperty
{
public:
  static vtkOpenGLProperty *New();
  vtkTypeMacro(vtkOpenGLProperty, vtkProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Implement base class method.
   */
  void Render(vtkActor *a, vtkRenderer *ren) override;

  /**
   * Implement base class method.
   */
  void BackfaceRender(vtkActor *a, vtkRenderer *ren) override;

  /**
   * This method is called after the actor has been rendered.
   * Don't call this directly. This method cleans up
   * any shaders allocated.
   */
  void PostRender(vtkActor *a,
                          vtkRenderer *r) override;

  /**
   * Release any graphics resources that are being consumed by this
   * property. The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *win) override;

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

#endif
