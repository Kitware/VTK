// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLRenderUtilities
 * @brief   OpenGL rendering utility functions
 *
 * vtkOpenGLRenderUtilities provides functions to help render primitives.
 *
 * See also the vtkOpenGLQuadHelper class which may be easier to use.
 *
 */

#ifndef vtkOpenGLRenderUtilities_h
#define vtkOpenGLRenderUtilities_h

#include "vtkObject.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

#include "vtk_glad.h" // Needed for GLuint.
#include <string>     // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLBufferObject;
class vtkOpenGLFramebufferObject;
class vtkOpenGLRenderWindow;
class vtkOpenGLState;
class vtkOpenGLVertexArrayObject;
class vtkRenderState;
class vtkShaderProgram;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLRenderUtilities : public vtkObject
{
public:
  vtkTypeMacro(vtkOpenGLRenderUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Helper function that draws a quad on the screen
   * at the specified vertex coordinates and if
   * tcoords are not NULL with the specified
   * texture coordinates.
   */
  static void RenderQuad(
    float* verts, float* tcoords, vtkShaderProgram* program, vtkOpenGLVertexArrayObject* vao);
  static void RenderTriangles(float* verts, unsigned int numVerts, GLuint* iboData,
    unsigned int numIndices, float* tcoords, vtkShaderProgram* program,
    vtkOpenGLVertexArrayObject* vao);
  ///@}

  /**
   * Composite a render pass result into the current draw framebuffer: blit it out of
   * readFramebuffer when that is legal, or draw colorTexture, its color attachment 0,
   * as a textured quad when the draw framebuffer is multisampled, which GLES forbids
   * blitting into. readFramebuffer is only bound, for reading, on the blit path. The
   * result lands on the (dstXmin, dstYmin, width, height) rectangle; the depth buffer
   * stays untouched on both paths. Returns true when the quad path was taken.
   */
  static bool CompositeColorTexture(vtkOpenGLState* state, const vtkRenderState* renderState,
    vtkOpenGLRenderWindow* renderWindow, vtkOpenGLFramebufferObject* readFramebuffer,
    vtkTextureObject* colorTexture, int dstXmin, int dstYmin, int width, int height);

  ///@{
  /**
   * Draw a full-screen quad:
   *
   * * VertexShader and GeometryShader should be used as-is when building the
   * ShaderProgram.
   * * FragmentShaderTemplate supports the replacements @code //VTK::FSQ::Decl @endcode and
   * @code //VTK::FSQ::Impl @endcode for declaring variables and the shader body,
   * respectively.
   * * The varying texCoord is available to the fragment shader for texture
   * lookups into full-screen textures, ie. texture2D(textureName, texCoord).
   * * PrepFullScreenVAO initializes a new VAO for drawing a quad.
   * * DrawFullScreenQuad actually draws the quad.

   * Example usage:
   * @code
   * typedef vtkOpenGLRenderUtilities GLUtil;

   * // Prep fragment shader source:
   * std::string fragShader = GLUtil::GetFullScreenQuadFragmentShaderTemplate();
   * vtkShaderProgram::Substitute(fragShader, "//VTK::FSQ::Decl",
   * "uniform sampler2D aTexture;");
   * vtkShaderProgram::Substitute(fragShader, "//VTK::FSQ::Impl",
   * "gl_FragData[0] = texture2D(aTexture, texCoord);");

   * // Create shader program:
   * vtkShaderProgram *prog = shaderCache->ReadyShaderProgram(
   * GLUtil::GetFullScreenQuadVertexShader().c_str(),
   * fragShader.c_str(),
   * GLUtil::GetFullScreenQuadGeometryShader().c_str());

   * // Initialize new VAO/vertex buffer. This is only done once:
   * vtkNew<vtkOpenGLVertexArrayObject> vao;
   * GLUtil::PrepFullScreenVAO(renWin, vao.Get(), prog);

   * // Setup shader program to sample vtkTextureObject aTexture:
   * aTexture->Activate();
   * prog->SetUniformi("aTexture", aTexture->GetTextureUnit());

   * // Render the full-screen quad:
   * vao->Bind();
   * GLUtil::DrawFullScreenQuad();
   * vao->Release();
   * aTexture->Deactivate();
   * @endcode
   */
  static std::string GetFullScreenQuadVertexShader();
  static std::string GetFullScreenQuadFragmentShaderTemplate();
  static std::string GetFullScreenQuadGeometryShader();
  static bool PrepFullScreenVAO(
    vtkOpenGLRenderWindow* renWin, vtkOpenGLVertexArrayObject* vao, vtkShaderProgram* prog);
  static void DrawFullScreenQuad();
  ///@}

  // older signsature, we suggest you use the newer signature above
  static bool PrepFullScreenVAO(
    vtkOpenGLBufferObject* vertBuf, vtkOpenGLVertexArrayObject* vao, vtkShaderProgram* prog);

  /**
   * Pass a debugging mark to the render engine to assist development via tools
   * like apitrace. This calls glDebugMessageInsert to insert the event string
   * into the OpenGL command stream.
   *
   * Note that this method only works when glDebugMessageInsert is bound, which
   * it may not be on certain platforms.
   */
  static void MarkDebugEvent(const std::string& event);

protected:
  vtkOpenGLRenderUtilities();
  ~vtkOpenGLRenderUtilities() override;

private:
  vtkOpenGLRenderUtilities(const vtkOpenGLRenderUtilities&) = delete;
  void operator=(const vtkOpenGLRenderUtilities&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
