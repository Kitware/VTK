// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLToneMappingPass
 * @brief   OpenGL implementation of vtkToneMappingPass.
 *
 * The delegate is rendered into a floating point framebuffer object, which is then drawn into the
 * current framebuffer with the selected tone mapping operator.
 *
 * @sa
 * vtkToneMappingPass
 */

#ifndef vtkOpenGLToneMappingPass_h
#define vtkOpenGLToneMappingPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkToneMappingPass.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLFramebufferObject;
class vtkOpenGLQuadHelper;
class vtkOverrideAttribute;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLToneMappingPass
  : public vtkToneMappingPass
{
public:
  static vtkOpenGLToneMappingPass* New();
  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkOpenGLToneMappingPass, vtkToneMappingPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state.
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own resources.
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

protected:
  vtkOpenGLToneMappingPass() = default;
  ~vtkOpenGLToneMappingPass() override;

  /**
   * Graphics resources.
   */
  vtkOpenGLFramebufferObject* FrameBufferObject = nullptr;
  vtkTextureObject* ColorTexture = nullptr;
  vtkOpenGLQuadHelper* QuadHelper = nullptr;

  /**
   * Used to recompile the shader if UseACES is modified
   */
  bool UseACESChangeValue = true;

private:
  vtkOpenGLToneMappingPass(const vtkOpenGLToneMappingPass&) = delete;
  void operator=(const vtkOpenGLToneMappingPass&) = delete;
};

#define vtkOpenGLToneMappingPass_OVERRIDE_ATTRIBUTES                                               \
  vtkOpenGLToneMappingPass::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
