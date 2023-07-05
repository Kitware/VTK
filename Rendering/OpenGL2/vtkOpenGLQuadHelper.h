// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLQuadHelper
 * @brief   Class to make rendering a full screen quad easier
 *
 * vtkOpenGLQuadHelper is designed to be used by classes in VTK that
 * need to render a quad to the screen with a shader program. This happens
 * often with render passes or other advanced rendering techniques.
 *
 * Note that when releasing graphics resources instances of this
 * class should be destroyed. A common use pattern is to conditionally
 * create the instance where used and delete it in ReleaseGraphicsResources
 * and the destructor.
 *
 * It is possible to flip the Y direction of the quad texture coordinate by
 * turning on the flipY option in the constructor. This can be useful when
 * rendering in an external context having a different convention than OpenGL
 * e.g. OpenGL-D3D shared textures. Off by default if unspecified.
 *
 * Example usage:
 * @code
 * if (!this->QuadHelper)
 * {
 *   this->QuadHelper = vtkOpenGLQualHelper(renWin, vs, fs, gs);
 * }
 * renWin->GetShaderCache()->ReadyShaderProgram(this->QuadHelper->Program);
 * aTexture->Activate();
 * this->QuadHelper->Program->SetUniformi("aTexture", aTexture->GetTextureUnit());
 * this->QuadHelper->Render();
 * aTexture->Deactivate();
 * @endcode
 *
 * @sa vtkOpenGLRenderUtilities
 */

#ifndef vtkOpenGLQuadHelper_h
#define vtkOpenGLQuadHelper_h

#include "vtkRenderingOpenGL2Module.h" // for export macro
#include "vtkTimeStamp.h"
#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;
class vtkOpenGLVertexArrayObject;
class vtkShaderProgram;
class vtkGenericOpenGLResourceFreeCallback;
class vtkWindow;

// Helper class to render full screen quads
class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLQuadHelper
{
public:
  vtkShaderProgram* Program;
  vtkTimeStamp ShaderSourceTime;
  vtkOpenGLVertexArrayObject* VAO;
  unsigned int ShaderChangeValue;

  /**
   * Create a quadhelper with the provided shaders.
   * If the vertex is nullptr then the default is used.
   * Turning on the flipY option reverts the y component of the quad texture coordinates in
   * order to flip the rendered texture.
   * Note that this class should be destroyed upon ReleaseGraphicsResources.
   */
  vtkOpenGLQuadHelper(
    vtkOpenGLRenderWindow*, const char* vs, const char* fs, const char* gs, bool flipY = false);

  ~vtkOpenGLQuadHelper();

  // Draw the Quad, will bind the VAO for you
  void Render();

  /**
   * Release graphics resources. In general, there's no need to call this
   * explicitly, since vtkOpenGLQuadHelper will invoke it appropriately when
   * needed.
   */
  void ReleaseGraphicsResources(vtkWindow*);

private:
  vtkOpenGLQuadHelper(const vtkOpenGLQuadHelper&) = delete;
  vtkOpenGLQuadHelper& operator=(const vtkOpenGLQuadHelper&) = delete;
  std::unique_ptr<vtkGenericOpenGLResourceFreeCallback> ResourceCallback;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLQuadHelper_h

// VTK-HeaderTest-Exclude: vtkOpenGLQuadHelper.h
