// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGaussianBlurPass
 * @brief   Implement a post-processing Gaussian blur
 * render pass.
 *
 * Blur the image rendered by its delegate. Blurring uses a Gaussian low-pass
 * filter with a 5x5 kernel.
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
 * As the filter is separable, it first blurs the image horizontally and then
 * vertically. This reduces the number of texture sampling to 5 per pass.
 * In addition, as texture sampling can already blend texel values in linear
 * mode, by adjusting the texture coordinate accordingly, only 3 texture
 * sampling are actually necessary.
 * Reference: OpenGL Bloom Toturial by Philip Rideout, section
 * Exploit Hardware Filtering  http://prideout.net/bloom/index.php#Sneaky
 *
 * @sa
 * vtkRenderPass
 */

#ifndef vtkGaussianBlurPass_h
#define vtkGaussianBlurPass_h

#include "vtkImageProcessingPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkDepthPeelingPassLayerList; // Pimpl
class vtkOpenGLFramebufferObject;
class vtkOpenGLHelper;
class vtkOpenGLRenderWindow;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkGaussianBlurPass : public vtkImageProcessingPass
{
public:
  static vtkGaussianBlurPass* New();
  vtkTypeMacro(vtkGaussianBlurPass, vtkImageProcessingPass);
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

protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  vtkGaussianBlurPass();

  /**
   * Destructor.
   */
  ~vtkGaussianBlurPass() override;

  /**
   * Graphics resources.
   */
  vtkOpenGLFramebufferObject* FrameBufferObject;
  vtkTextureObject* Pass1; // render target for the scene
  vtkTextureObject* Pass2; // render target for the horizontal pass

  // Structures for the various cell types we render.
  vtkOpenGLHelper* BlurProgram;

private:
  vtkGaussianBlurPass(const vtkGaussianBlurPass&) = delete;
  void operator=(const vtkGaussianBlurPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
