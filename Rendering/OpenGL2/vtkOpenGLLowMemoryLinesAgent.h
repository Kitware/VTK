// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLLowMemoryLinesAgent
 * @brief   Maps VTK_LINE and VTK_POLY_LINE into GL_LINES and draws GL_LINES or GL_POINTS.
 */

#ifndef vtkOpenGLLowMemoryLinesAgent_h
#define vtkOpenGLLowMemoryLinesAgent_h

#include "vtkOpenGLLowMemoryCellTypeAgent.h"

#include "vtkRenderingOpenGL2Module.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_NO_EXPORT vtkOpenGLLowMemoryLinesAgent
  : public vtkOpenGLLowMemoryCellTypeAgent
{
public:
  vtkOpenGLLowMemoryLinesAgent();
  ~vtkOpenGLLowMemoryLinesAgent() override;

  bool ImplementsVertexVisibilityPass() const override { return true; }

protected:
  void PreDrawInternal(vtkRenderer* renderer, vtkActor* actor,
    vtkOpenGLLowMemoryPolyDataMapper* lmMapper) const override;
  void PostDrawInternal(vtkRenderer* renderer, vtkActor* actor,
    vtkOpenGLLowMemoryPolyDataMapper* lmMapper) const override;
};

VTK_ABI_NAMESPACE_END
#endif

// Need to skip header testing since we do not inherit vtkObject:
// VTK-HeaderTest-Exclude: vtkOpenGLLowMemoryLinesAgent.h
