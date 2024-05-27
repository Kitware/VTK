// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLFXAAPass
 * @brief   Render pass calling the FXAA filter
 *
 * vtkOpenGLFXAAPass is an image post processing render pass. It is a fast anti aliasing
 * filter.
 *
 * This pass usually takes the camera pass as its delegate pass.
 *
 * @note Currently, this pass wraps the existing FXAA implementation. It copies the pixels
 * from the framebuffer to a texture. A better approach would be to use the usual render pass
 * workflow to create a framebuffer drawing directly on the texture.
 *
 * @sa
 * vtkRenderPass vtkDefaultPass
 */

#ifndef vtkOpenGLFXAAPass_h
#define vtkOpenGLFXAAPass_h

#include "vtkImageProcessingPass.h"

#include "vtkNew.h"                    // For vtkNew
#include "vtkOpenGLFXAAFilter.h"       // For vtkOpenGLFXAAFilter
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkFXAAOptions;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLFXAAPass : public vtkImageProcessingPass
{
public:
  static vtkOpenGLFXAAPass* New();
  vtkTypeMacro(vtkOpenGLFXAAPass, vtkImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state.
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  vtkGetObjectMacro(FXAAOptions, vtkFXAAOptions);
  virtual void SetFXAAOptions(vtkFXAAOptions*);

protected:
  vtkOpenGLFXAAPass() = default;
  ~vtkOpenGLFXAAPass() override;

  /**
   * Graphics resources.
   */
  vtkNew<vtkOpenGLFXAAFilter> FXAAFilter;

  vtkFXAAOptions* FXAAOptions = nullptr;

private:
  vtkOpenGLFXAAPass(const vtkOpenGLFXAAPass&) = delete;
  void operator=(const vtkOpenGLFXAAPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
