// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStencilMaskPass
 * @brief   Implements a render pass for geometric stencil masking.
 *
 * vtkStencilMaskPass allows you to use specific actors to occlude specific regions of a scene.
 *
 * This pass utilizes the OpenGL stencil buffer to generate a mask. When executed,
 * it renders all actors with the `vtkStencilMaskPass::GLStencilWrite()`
 * information key into the stencil buffer, defining the masked area.
 * Any subsequent rendering pass executed after this one will be constrained
 * by this mask, drawing only in unstenciled regions.
 *
 * Please note that this pass also writes to the Depth Buffer while
 * rendering the masking geometry. If you want subsequent render passes to ignore
 * the depth of the mask itself, you should clear the depth buffer immediately
 * after this pass by injecting a `vtkClearZPass` into your render pipeline.
 *
 * @warning For this pass to work, the render window must be configured to allocate
 * a stencil buffer by calling `StencilCapableOn()` on your `vtkRenderWindow`.
 *
 * @sa
 * vtkRenderPass vtkClearZPass vtkRenderWindow
 */

#ifndef vtkStencilMaskPass_h
#define vtkStencilMaskPass_h

#include "vtkRenderPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationIntegerKey;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkStencilMaskPass : public vtkRenderPass
{
public:
  static vtkStencilMaskPass* New();
  vtkTypeMacro(vtkStencilMaskPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state.
   */
  void Render(const vtkRenderState* state) override;

  /**
   * Whether or not we want to write an actor into the stencil buffer
   * A value of 1 means enabled, 0 disabled.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* GLStencilWrite();

protected:
  vtkStencilMaskPass() = default;
  ~vtkStencilMaskPass() override;

private:
  vtkStencilMaskPass(const vtkStencilMaskPass&) = delete;
  void operator=(const vtkStencilMaskPass&) = delete;

  bool AlreadyWarnedAboutStencils = false; // Avoid spam when user interact with the window
};

VTK_ABI_NAMESPACE_END
#endif
