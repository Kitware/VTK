// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLES30PolyDataMapper2D
 * @brief   2D PolyData support for OpenGL ES 3.0
 *
 * This mapper provides a GLES 3.0 compatible implementation of the 2D OpenGL
 * polydata mapper. Since GLES30 3.0 lacks geometry shaders and texture buffers,
 * `vtkOpenGLPolyDataMapper2D` will not function
 * correctly when VTK is configured with `VTK_OPENGL_USE_GLES=ON` since that mapper
 * works with GLES30 >= 3.2 or desktop GL 3.2 contexts.
 *
 * @note This class replaces the default OpenGL factory override for `vtkOpenGLPolyDataMapper2D`
 * when VTK targets GLES 3.0 contexts with `VTK_OPENGL_USE_GLES=ON`.
 *
 * @sa
 * vtkPolyDataMapper2D, vtkOpenGLPolyDataMapper2D
 */

#ifndef vtkOpenGLES30PolyDataMapper2D_h
#define vtkOpenGLES30PolyDataMapper2D_h

#include "vtkOpenGLPolyDataMapper2D.h"

#include "vtkOpenGLVertexBufferObjectGroup.h" // for ivar
#include "vtkRenderingOpenGL2Module.h"        // for export macro
#include "vtkWrappingHints.h"                 // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLES30PolyDataMapper2D
  : public vtkOpenGLPolyDataMapper2D
{
public:
  vtkTypeMacro(vtkOpenGLES30PolyDataMapper2D, vtkOpenGLPolyDataMapper2D);
  static vtkOpenGLES30PolyDataMapper2D* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum PrimitiveTypes
  {
    PrimitiveStart = 0,
    PrimitivePoints = 0,
    PrimitiveLines,
    PrimitiveTris,
    PrimitiveTriStrips,
    PrimitiveEnd
  };

  /**
   * Actually draw the poly data.
   */
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

protected:
  vtkOpenGLES30PolyDataMapper2D();
  ~vtkOpenGLES30PolyDataMapper2D() override;

  /**
   * Build the shader source code
   */
  void BuildShaders(std::string& VertexCode, std::string& fragmentCode, std::string& geometryCode,
    vtkViewport* ren, vtkActor2D* act) override;

  /**
   * In GLES 3.0, point size is set from the vertex shader.
   */
  void ReplaceShaderPointSize(std::string& VSSource, vtkViewport* ren, vtkActor2D* act);

  /**
   * GLES 3.0 does not support wide lines (width > 1). Shader computations combined with
   * instanced rendering is used to emulate wide lines.
   */
  void ReplaceShaderWideLines(std::string& VSSource, vtkViewport* ren, vtkActor2D* act);

  /**
   * Determine what shader to use and compile/link it
   */
  void UpdateShaders(vtkOpenGLHelper& cellBO, vtkViewport* viewport, vtkActor2D* act) override;

  /**
   * Set the shader parameters related to the mapper/input data, called by UpdateShader
   */
  void SetMapperShaderParameters(
    vtkOpenGLHelper& cellBO, vtkViewport* viewport, vtkActor2D* act) override;

  /**
   * Update the scene when necessary.
   */
  void UpdateVBO(vtkActor2D* act, vtkViewport* viewport);

  std::vector<unsigned int> PrimitiveIndexArrays[PrimitiveEnd];
  vtkNew<vtkOpenGLVertexBufferObjectGroup> PrimitiveVBOGroup[PrimitiveEnd];
  PrimitiveTypes CurrentDrawCallPrimtiveType = PrimitiveEnd;

private:
  vtkOpenGLES30PolyDataMapper2D(const vtkOpenGLES30PolyDataMapper2D&) = delete;
  void operator=(const vtkOpenGLES30PolyDataMapper2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
