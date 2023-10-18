// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLLowMemoryPolygonsAgent
 * @brief   Maps VTK_TRIANGLE and VTK_POLYGON into GL_TRIANGLES and draws GL_TRIANGLES or GL_LINES
 * or GL_POINTS.
 */

#ifndef vtkOpenGLLowMemoryPolygonsAgent_h
#define vtkOpenGLLowMemoryPolygonsAgent_h

#include "vtkOpenGLLowMemoryCellTypeAgent.h"

#include "vtkCellGraphicsPrimitiveMap.h"
#include "vtkRenderingOpenGL2Module.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_NO_EXPORT vtkOpenGLLowMemoryPolygonsAgent
  : public vtkOpenGLLowMemoryCellTypeAgent
{
public:
  vtkOpenGLLowMemoryPolygonsAgent();
  ~vtkOpenGLLowMemoryPolygonsAgent() override;

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
// VTK-HeaderTest-Exclude: vtkOpenGLLowMemoryPolygonsAgent.h
