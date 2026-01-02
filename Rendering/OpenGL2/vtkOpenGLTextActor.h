// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkOpenGLTextActor
 * @brief   vtkTextActor override.
 */

#ifndef vtkOpenGLTextActor_h
#define vtkOpenGLTextActor_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkTextActor.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLGL2PSHelper;
class vtkOverrideAttribute;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLTextActor : public vtkTextActor
{
public:
  static vtkOpenGLTextActor* New();
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkOpenGLTextActor, vtkTextActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int RenderOverlay(vtkViewport* viewport) override;

protected:
  vtkOpenGLTextActor();
  ~vtkOpenGLTextActor() override;

  int RenderGL2PS(vtkViewport* viewport, vtkOpenGLGL2PSHelper* gl2ps);

private:
  vtkOpenGLTextActor(const vtkOpenGLTextActor&) = delete;
  void operator=(const vtkOpenGLTextActor&) = delete;
};

#define vtkOpenGLTextActor_OVERRIDE_ATTRIBUTES vtkOpenGLTextActor::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLTextActor_h
