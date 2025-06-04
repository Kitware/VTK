// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkOpenGLTextActor3D
 * @brief   OpenGL2 override for vtkTextActor3D.
 */

#ifndef vtkOpenGLTextActor3D_h
#define vtkOpenGLTextActor3D_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkTextActor3D.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLGL2PSHelper;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLTextActor3D : public vtkTextActor3D
{
public:
  static vtkOpenGLTextActor3D* New();
  vtkTypeMacro(vtkOpenGLTextActor3D, vtkTextActor3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) override;

protected:
  vtkOpenGLTextActor3D();
  ~vtkOpenGLTextActor3D() override;

  int RenderGL2PS(vtkViewport* vp, vtkOpenGLGL2PSHelper* gl2ps);

private:
  vtkOpenGLTextActor3D(const vtkOpenGLTextActor3D&) = delete;
  void operator=(const vtkOpenGLTextActor3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLTextActor3D_h
