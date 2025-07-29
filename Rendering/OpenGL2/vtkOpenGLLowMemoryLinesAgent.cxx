// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLLowMemoryLinesAgent.h"
#include "vtkActor.h"
#include "vtkCellType.h"
#include "vtkOpenGLLowMemoryPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkShaderProgram.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkOpenGLLowMemoryLinesAgent::vtkOpenGLLowMemoryLinesAgent()
{
  // agent draws polygons as a collection of line segments.
  this->NumberOfPointsPerPrimitive = 2;
}

//------------------------------------------------------------------------------
vtkOpenGLLowMemoryLinesAgent::~vtkOpenGLLowMemoryLinesAgent() = default;

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryLinesAgent::PreDrawInternal(
  vtkRenderer*, vtkActor* actor, vtkOpenGLLowMemoryPolyDataMapper* lmMapper) const
{
  lmMapper->ElementType = vtkDrawTexturedElements::ElementShape::Line;
  if (actor->GetProperty()->GetLineWidth() > 1 &&
    actor->GetProperty()->GetRepresentation() != VTK_POINTS)
  {
    this->NumberOfPseudoPrimitivesPerElement = 2; // Each line segment is drawn as 2 triangles.
    lmMapper->ElementType = vtkDrawTexturedElements::ElementShape::Triangle;
  }
  else
  {
    this->NumberOfPseudoPrimitivesPerElement = 1;
    lmMapper->ElementType = vtkDrawTexturedElements::ElementShape::Line;
  }
  lmMapper->ShaderProgram->SetUniformi("cellType", VTK_LINE);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryLinesAgent::PostDrawInternal(
  vtkRenderer*, vtkActor*, vtkOpenGLLowMemoryPolyDataMapper*) const
{
}

VTK_ABI_NAMESPACE_END
