// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLLowMemoryCellTypeAgent
 * @brief   Helps vtkOpenGLLowMemoryPolyDataMapper map and draw cell types from vtkPolyData as
 * OpenGL graphics primitives.
 */

#ifndef vtkOpenGLLowMemoryCellTypeAgent_h
#define vtkOpenGLLowMemoryCellTypeAgent_h

#include "vtkCellGraphicsPrimitiveMap.h"
#include "vtkOpenGLLowMemoryPolyDataMapper.h"
#include "vtkRenderingOpenGL2Module.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkRenderer;

class VTKRENDERINGOPENGL2_NO_EXPORT vtkOpenGLLowMemoryCellTypeAgent
{
public:
  vtkOpenGLLowMemoryCellTypeAgent();
  virtual ~vtkOpenGLLowMemoryCellTypeAgent();

  void PreDraw(
    vtkRenderer* renderer, vtkActor* actor, vtkOpenGLLowMemoryPolyDataMapper* mapper) const;
  void Draw(vtkRenderer* renderer, vtkActor* actor, vtkOpenGLLowMemoryPolyDataMapper* mapper,
    const std::vector<vtkOpenGLLowMemoryPolyDataMapper::CellGroupInformation>& cellGroups,
    std::size_t cellGroupIdx = 0) const;
  void PostDraw(
    vtkRenderer* renderer, vtkActor* actor, vtkOpenGLLowMemoryPolyDataMapper* mapper) const;

  virtual bool ImplementsVertexVisibilityPass() const = 0;
  void BeginVertexVisibilityPass() { this->InVertexVisibilityPass = true; }
  void EndVertexVisibilityPass() { this->InVertexVisibilityPass = false; }

protected:
  virtual void PreDrawInternal(
    vtkRenderer* renderer, vtkActor* actor, vtkOpenGLLowMemoryPolyDataMapper* lmMapper) const = 0;
  virtual void PostDrawInternal(
    vtkRenderer* renderer, vtkActor* actor, vtkOpenGLLowMemoryPolyDataMapper* lmMapper) const = 0;

  int NumberOfPointsPerPrimitive = 1;
  bool InVertexVisibilityPass = false;
  mutable int NumberOfPseudoPrimitivesPerElement =
    1; // Used to track how many pseudo primitives are used for each element type.
};

VTK_ABI_NAMESPACE_END
#endif

// Need to skip header testing since we do not inherit vtkObject:
// VTK-HeaderTest-Exclude: vtkOpenGLLowMemoryCellTypeAgent.h
