// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageProcessingPass
 * @brief   Convenient class for post-processing passes.
 * render pass.
 *
 * Abstract class with some convenient methods frequently used in subclasses.
 *
 *
 * @sa
 * vtkOpenGLRenderPass vtkGaussianBlurPass vtkSobelGradientMagnitudePass
 */

#ifndef vtkImageProcessingPass_h
#define vtkImageProcessingPass_h

#include "vtkOpenGLRenderPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;
class vtkDepthPeelingPassLayerList; // Pimpl
class vtkOpenGLFramebufferObject;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkImageProcessingPass : public vtkOpenGLRenderPass
{
public:
  vtkTypeMacro(vtkImageProcessingPass, vtkOpenGLRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  ///@{
  /**
   * Delegate for rendering the image to be processed.
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * It is usually set to a vtkCameraPass or to a post-processing pass.
   * Initial value is a NULL pointer.
   */
  vtkGetObjectMacro(DelegatePass, vtkRenderPass);
  virtual void SetDelegatePass(vtkRenderPass* delegatePass);
  ///@}

protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  vtkImageProcessingPass();

  /**
   * Destructor.
   */
  ~vtkImageProcessingPass() override;

  /**
   * Render delegate with a image of different dimensions than the
   * original one.
   * \pre s_exists: s!=0
   * \pre fbo_exists: fbo!=0
   * \pre fbo_has_context: fbo->GetContext()!=0
   * \pre target_exists: target!=0
   * \pre target_has_context: target->GetContext()!=0
   */
  void RenderDelegate(const vtkRenderState* s, int width, int height, int newWidth, int newHeight,
    vtkOpenGLFramebufferObject* fbo, vtkTextureObject* target);

  vtkRenderPass* DelegatePass;

private:
  vtkImageProcessingPass(const vtkImageProcessingPass&) = delete;
  void operator=(const vtkImageProcessingPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
