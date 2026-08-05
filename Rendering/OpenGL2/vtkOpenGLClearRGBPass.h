// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLClearRGBPass
 * @brief   OpenGL clear RGB pass.
 *
 * vtkOpenGLClearRGBPass is the OpenGL implementation of vtkClearRGBPass.
 *
 * @sa
 * vtkClearRGBPass
 */

#ifndef vtkOpenGLClearRGBPass_h
#define vtkOpenGLClearRGBPass_h

#include "vtkClearRGBPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOverrideAttribute;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLClearRGBPass : public vtkClearRGBPass
{
public:
  static vtkOpenGLClearRGBPass* New();
  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkOpenGLClearRGBPass, vtkClearRGBPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state s.
   */
  void Render(const vtkRenderState* s) override;

protected:
  vtkOpenGLClearRGBPass();
  ~vtkOpenGLClearRGBPass() override;

private:
  vtkOpenGLClearRGBPass(const vtkOpenGLClearRGBPass&) = delete;
  void operator=(const vtkOpenGLClearRGBPass&) = delete;
};

#define vtkOpenGLClearRGBPass_OVERRIDE_ATTRIBUTES vtkOpenGLClearRGBPass::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
