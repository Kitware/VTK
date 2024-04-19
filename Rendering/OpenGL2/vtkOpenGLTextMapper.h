// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkOpenGLTextMapper
 * @brief   vtkTextMapper override for OpenGL2.
 */

#ifndef vtkOpenGLTextMapper_h
#define vtkOpenGLTextMapper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkTextMapper.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLGL2PSHelper;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLTextMapper : public vtkTextMapper
{
public:
  static vtkOpenGLTextMapper* New();
  vtkTypeMacro(vtkOpenGLTextMapper, vtkTextMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void RenderOverlay(vtkViewport* vp, vtkActor2D* act) override;

protected:
  vtkOpenGLTextMapper();
  ~vtkOpenGLTextMapper() override;

  void RenderGL2PS(vtkViewport* vp, vtkActor2D* act, vtkOpenGLGL2PSHelper* gl2ps);

private:
  vtkOpenGLTextMapper(const vtkOpenGLTextMapper&) = delete;
  void operator=(const vtkOpenGLTextMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLTextMapper_h
