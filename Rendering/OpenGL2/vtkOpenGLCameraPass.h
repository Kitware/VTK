// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLCameraPass
 * @brief   OpenGL camera pass.
 *
 * vtkOpenGLCameraPass is the OpenGL implementation of vtkCameraPass.
 *
 * @sa
 * vtkCameraPass
 */

#ifndef vtkOpenGLCameraPass_h
#define vtkOpenGLCameraPass_h

#include "vtkCameraPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOverrideAttribute;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLCameraPass : public vtkCameraPass
{
public:
  static vtkOpenGLCameraPass* New();
  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkOpenGLCameraPass, vtkCameraPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState* s) override;

protected:
  vtkOpenGLCameraPass();
  ~vtkOpenGLCameraPass() override;

private:
  vtkOpenGLCameraPass(const vtkOpenGLCameraPass&) = delete;
  void operator=(const vtkOpenGLCameraPass&) = delete;
};

#define vtkOpenGLCameraPass_OVERRIDE_ATTRIBUTES vtkOpenGLCameraPass::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
