// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOrderIndependentTranslucentPass
 * @brief   Implement OIT rendering using average color
 *
 * Simple version that uses average alpha weighted color
 * and correct final computed alpha. Single pass approach.
 *
 * @sa
 * vtkRenderPass, vtkTranslucentPass, vtkFramebufferPass
 */

#ifndef vtkOrderIndependentTranslucentPass_h
#define vtkOrderIndependentTranslucentPass_h

#include "vtkOpenGLRenderPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLFramebufferObject;
class vtkTextureObject;
class vtkOpenGLRenderWindow;
class vtkOpenGLState;
class vtkOpenGLQuadHelper;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOrderIndependentTranslucentPass
  : public vtkOpenGLRenderPass
{
public:
  static vtkOrderIndependentTranslucentPass* New();
  vtkTypeMacro(vtkOrderIndependentTranslucentPass, vtkOpenGLRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  ///@{
  /**
   * Delegate for rendering the translucent polygonal geometry.
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * It is usually set to a vtkTranslucentPass.
   * Initial value is a NULL pointer.
   */
  vtkGetObjectMacro(TranslucentPass, vtkRenderPass);
  virtual void SetTranslucentPass(vtkRenderPass* translucentPass);
  ///@}

  // vtkOpenGLRenderPass virtuals:
  bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop) override;

protected:
  /**
   * Default constructor. TranslucentPass is set to NULL.
   */
  vtkOrderIndependentTranslucentPass();

  /**
   * Destructor.
   */
  ~vtkOrderIndependentTranslucentPass() override;

  vtkRenderPass* TranslucentPass;

  ///@{
  /**
   * Cache viewport values for depth peeling.
   */
  int ViewportX;
  int ViewportY;
  int ViewportWidth;
  int ViewportHeight;
  ///@}

  vtkOpenGLFramebufferObject* Framebuffer;
  vtkOpenGLQuadHelper* FinalBlend;

  vtkTextureObject* TranslucentRGBATexture;
  vtkTextureObject* TranslucentRTexture;
  vtkTextureObject* TranslucentZTexture;

  void BlendFinalPeel(vtkOpenGLRenderWindow* renWin);

  // useful to store
  vtkOpenGLState* State;

private:
  vtkOrderIndependentTranslucentPass(const vtkOrderIndependentTranslucentPass&) = delete;
  void operator=(const vtkOrderIndependentTranslucentPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
