// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkOpenGLRenderPass
 * @brief   Abstract render pass with shader modifications.
 *
 *
 * Allows a render pass to update shader code using a new virtual API.
 */

#ifndef vtkOpenGLRenderPass_h
#define vtkOpenGLRenderPass_h

#include "vtkRenderPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

#include <string> // For std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractMapper;
class vtkInformationObjectBaseVectorKey;
class vtkProp;
class vtkShaderProgram;
class vtkOpenGLVertexArrayObject;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLRenderPass : public vtkRenderPass
{
public:
  vtkTypeMacro(vtkOpenGLRenderPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Use vtkShaderProgram::Substitute to replace @code //VTK::XXX:YYY @endcode declarations in
   * the shader sources. Gets called before other mapper shader replacements
   * Return false on error.
   */
  virtual bool PreReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop);
  virtual bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop);
  ///@}

  /**
   * Update the uniforms of the shader program.
   * Return false on error.
   */
  virtual bool SetShaderParameters(vtkShaderProgram* program, vtkAbstractMapper* mapper,
    vtkProp* prop, vtkOpenGLVertexArrayObject* VAO = nullptr);

  /**
   * For multi-stage render passes that need to change shader code during a
   * single pass, use this method to notify a mapper that the shader needs to be
   * rebuilt (rather than reuse the last cached shader. This method should
   * return the last time that the shader stage changed, or 0 if the shader
   * is single-stage.
   */
  virtual vtkMTimeType GetShaderStageMTime();

  /**
   * Key containing information about the current pass.
   */
  static vtkInformationObjectBaseVectorKey* RenderPasses();

  /**
   * Number of active draw buffers.
   */
  vtkSetMacro(ActiveDrawBuffers, unsigned int);
  vtkGetMacro(ActiveDrawBuffers, unsigned int);

protected:
  vtkOpenGLRenderPass();
  ~vtkOpenGLRenderPass() override;

  /**
   * Call before rendering to update the actors' information keys.
   */
  void PreRender(const vtkRenderState* s);

  /**
   * Called in PreRender to give a chance to subclasses to set additional information keys.
   * This will be called for each filtered prop in the state.
   */
  virtual void PreRenderProp(vtkProp* prop);

  /**
   * Call after rendering to clean up the actors' information keys.
   */
  void PostRender(const vtkRenderState* s);

  /**
   * Called in PreRender to give a chance to subclasses to clean up information keys.
   * This will be called for each filtered prop in the state.
   */
  virtual void PostRenderProp(vtkProp* prop);

  unsigned int ActiveDrawBuffers = 0;

private:
  vtkOpenGLRenderPass(const vtkOpenGLRenderPass&) = delete;
  void operator=(const vtkOpenGLRenderPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLRenderPass_h
