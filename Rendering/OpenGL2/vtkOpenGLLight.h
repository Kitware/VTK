// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLLight
 * @brief   OpenGL light
 *
 * vtkOpenGLLight is a concrete implementation of the abstract class vtkLight.
 * vtkOpenGLLight interfaces to the OpenGL rendering library.
 */

#ifndef vtkOpenGLLight_h
#define vtkOpenGLLight_h

#include "vtkLight.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderer;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLLight : public vtkLight
{
public:
  static vtkOpenGLLight* New();
  vtkTypeMacro(vtkOpenGLLight, vtkLight);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Implement base class method.
   */
  void Render(vtkRenderer* ren, int light_index) override;

protected:
  vtkOpenGLLight() = default;
  ~vtkOpenGLLight() override = default;

private:
  vtkOpenGLLight(const vtkOpenGLLight&) = delete;
  void operator=(const vtkOpenGLLight&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
