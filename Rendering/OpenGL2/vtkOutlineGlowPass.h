// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOutlineGlowPass
 * @brief   Renders a glowing outline using a image processing pass
 *
 * Create a glowing outline of the image rendered by the delegate.
 *
 * This render pass was designed to highlight parts of a scene by applying the render pass to a
 * layered renderer on top of the main scene. For optimal results, actors that form the outline
 * should be brightly colored with lighting disabled. The outline will have the color of the actors.
 * There is only one outline around all objects rendered by the delegate.
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farthest z-value and
 * background color/gradient/transparent color.
 * An opaque pass may have been performed right after the initialization.
 *
 * The delegate is used once.
 *
 * Its delegate is usually set to a vtkCameraPass or to a post-processing pass.
 *
 * This pass requires a OpenGL context that supports texture objects (TO),
 * framebuffer objects (FBO) and GLSL. If not, it will emit an error message
 * and will render its delegate and return.
 *
 * @par Implementation:
 * The image is first rendered to a full size offscreen render target, then blurred twice on a half
 * sized render target using Gaussian blur with an offset. The offset and the smaller render target
 * increase the size of the outline without incurring the cost of a big Gaussian blur kernel. The
 * implementation of the gaussian blur is similar to vtkGaussianBlurPass with the alterations
 * described above.
 *
 * @sa
 * vtkRenderPass vtkGaussianBlurPass
 */

#ifndef vtkOutlineGlowPass_h
#define vtkOutlineGlowPass_h

#include "vtkImageProcessingPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLFramebufferObject;
class vtkOpenGLHelper;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOutlineGlowPass : public vtkImageProcessingPass
{
public:
  static vtkOutlineGlowPass* New();
  vtkTypeMacro(vtkOutlineGlowPass, vtkImageProcessingPass);
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

  /**
   * Get/Set the intensity of the outline.
   * Default value is 3.0 which gives a bright outline with a fading edge
   */
  vtkGetMacro(OutlineIntensity, float);
  vtkSetMacro(OutlineIntensity, float);

protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  vtkOutlineGlowPass();

  /**
   * Destructor.
   */
  ~vtkOutlineGlowPass() override;

  /**
   * Graphics resources.
   */
  vtkOpenGLFramebufferObject* FrameBufferObject;
  vtkTextureObject* ScenePass; // render target for the original scene
  vtkTextureObject* BlurPass1; // render target for vertical blur
  vtkTextureObject* BlurPass2; // render target for horizontal blur

  // Shader programs
  vtkOpenGLHelper* BlurProgram;
  vtkOpenGLHelper* UpscaleProgram;

  // Default value of 3.0 gives a bright outline with a fading edge
  float OutlineIntensity = 3.0f;

private:
  vtkOutlineGlowPass(const vtkOutlineGlowPass&) = delete;
  void operator=(const vtkOutlineGlowPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkOutlineGlowPass_h */
