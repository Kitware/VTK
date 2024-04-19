// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLLowMemoryVerticesAgent
 * @brief   Maps VTK_VERTEX and VTK_POLY_VERTEX into GL_POINTS and draws GL_POINTS.
 */

#ifndef vtkOpenGLLowMemoryVerticesAgent_h
#define vtkOpenGLLowMemoryVerticesAgent_h

#include "vtkOpenGLLowMemoryCellTypeAgent.h"

#include "vtkPolyData.h"
#include "vtkRenderingOpenGL2Module.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_NO_EXPORT vtkOpenGLLowMemoryVerticesAgent
  : public vtkOpenGLLowMemoryCellTypeAgent
{
public:
  vtkOpenGLLowMemoryVerticesAgent();
  ~vtkOpenGLLowMemoryVerticesAgent() override;

  bool ImplementsVertexVisibilityPass() const override { return false; }

protected:
  void PreDrawInternal(vtkRenderer* renderer, vtkActor* actor,
    vtkOpenGLLowMemoryPolyDataMapper* lmMapper) const override;
  void PostDrawInternal(vtkRenderer* renderer, vtkActor* actor,
    vtkOpenGLLowMemoryPolyDataMapper* lmMapper) const override;
};

VTK_ABI_NAMESPACE_END
#endif

// Need to skip header testing since we do not inherit vtkObject:
// VTK-HeaderTest-Exclude: vtkOpenGLLowMemoryVerticesAgent.h
