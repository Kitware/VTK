// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSSAAPass
 * @brief   Implement Screen Space Anti Aliasing pass.
 *
 * Render to a larger image and then sample down
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farthest z-value and
 * background color/gradient/transparent color.
 *
 * The delegate is used once.
 *
 * Its delegate is usually set to a vtkCameraPass or to a post-processing pass.
 *
 * @par Implementation:
 * As the filter is separable, it first blurs the image horizontally and then
 * vertically. This reduces the number of texture samples taken.
 *
 * @sa
 * vtkRenderPass
 */

#ifndef vtkSSAAPass_h
#define vtkSSAAPass_h

#include "vtkRenderPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSmartPointer.h"           // for vtkSmartPointer
#include "vtkTextureObject.h"          // for depth/color format
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

#include <memory> // for unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLFramebufferObject;
class vtkOpenGLHelper;
class vtkRenderbuffer;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkSSAAPass : public vtkRenderPass
{
public:
  static vtkSSAAPass* New();
  vtkTypeMacro(vtkSSAAPass, vtkRenderPass);
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
   * Delegate for rendering the image to be processed.
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * It is usually set to a vtkCameraPass or to a post-processing pass.
   * Initial value is a NULL pointer.
   */
  vtkGetObjectMacro(DelegatePass, vtkRenderPass);
  virtual void SetDelegatePass(vtkRenderPass* delegatePass);
  ///@}

  ///@{
  /**
   * Set/Get the format to use for the color texture.
   * vtkTextureObject::Float16, vtkTextureObject::Float32
   * and vtkTextureObject::Fixed8 are supported.
   * Fixed8 is the default.
   */
  vtkSetMacro(ColorFormat, int);
  vtkGetMacro(ColorFormat, int);
  ///@}

  ///@{
  /**
   * Set/Get the format to use for the internal depth textures.
   * vtkTextureObject::Fixed16, vtkTextureObject::Fixed24
   * and vtkTextureObject::Fixed32 are supported.
   * Fixed24 is the default.
   * @note This depth format is enforced only when the render window is NOT stencil capable.
   * If the render window is stencil capable, the depth format in use will be depth:24 bits
   * and stencil:8 bits
   */
  vtkSetMacro(DepthFormat, int);
  vtkGetMacro(DepthFormat, int);
  ///@}

protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  vtkSSAAPass();

  /**
   * Destructor.
   */
  ~vtkSSAAPass() override;

  /**
   * Graphics resources.
   */
  vtkSmartPointer<vtkOpenGLFramebufferObject> FrameBufferObject;
  vtkSmartPointer<vtkTextureObject> Pass1; // render target for the scene (modifiedW x modifiedH)
  vtkSmartPointer<vtkTextureObject> Pass2; // render target for the horizontal pass (W x modifiedH)
  vtkSmartPointer<vtkTextureObject> DepthTexture1; // depth target paired with Pass1
  vtkSmartPointer<vtkTextureObject> DepthTexture2; // depth target paired with Pass2

  // Structures for the various cell types we render.
  std::unique_ptr<vtkOpenGLHelper> SSAAHelper;

  vtkSmartPointer<vtkRenderPass> DelegatePass;

private:
  vtkSSAAPass(const vtkSSAAPass&) = delete;
  void operator=(const vtkSSAAPass&) = delete;

  int ColorFormat = vtkTextureObject::Fixed8;  // framebuffer color texture format
  int DepthFormat = vtkTextureObject::Fixed24; // framebuffer depth texture format
};

VTK_ABI_NAMESPACE_END
#endif
