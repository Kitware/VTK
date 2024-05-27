// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLImageMapper
 * @brief   2D image display support for OpenGL
 *
 * vtkOpenGLImageMapper is a concrete subclass of vtkImageMapper that
 * renders images under OpenGL
 *
 * @warning
 * vtkOpenGLImageMapper does not support vtkBitArray, you have to convert the array first
 * to vtkUnsignedCharArray (for example)
 *
 * @sa
 * vtkImageMapper
 */

#ifndef vtkOpenGLImageMapper_h
#define vtkOpenGLImageMapper_h

#include "vtkImageMapper.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor2D;
class vtkTexturedActor2D;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLImageMapper : public vtkImageMapper
{
public:
  static vtkOpenGLImageMapper* New();
  vtkTypeMacro(vtkOpenGLImageMapper, vtkImageMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Handle the render method.
   */
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor) override
  {
    this->RenderStart(viewport, actor);
  }

  /**
   * Called by the Render function in vtkImageMapper.  Actually draws
   * the image to the screen.
   */
  void RenderData(vtkViewport* viewport, vtkImageData* data, vtkActor2D* actor) override;

  /**
   * draw the data once it has been converted to uchar, windowed leveled
   * used internally by the templated functions
   */
  void DrawPixels(vtkViewport* viewport, int width, int height, int numComponents, void* data);

  /**
   * Release any graphics resources that are being consumed by this
   * mapper, the image texture in particular.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

protected:
  vtkOpenGLImageMapper();
  ~vtkOpenGLImageMapper() override;

  vtkTexturedActor2D* Actor;

private:
  vtkOpenGLImageMapper(const vtkOpenGLImageMapper&) = delete;
  void operator=(const vtkOpenGLImageMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
