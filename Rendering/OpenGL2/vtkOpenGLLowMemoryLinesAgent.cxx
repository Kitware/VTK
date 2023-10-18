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
  if (actor->GetProperty()->GetLineWidth() > 1)
  {
    lmMapper->NumberOfInstances = 2 * vtkMath::Ceil(actor->GetProperty()->GetLineWidth());
  }
  else
  {
    lmMapper->NumberOfInstances = 1;
  }
  lmMapper->ShaderProgram->SetUniformi("cellType", VTK_LINE);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryLinesAgent::PostDrawInternal(
  vtkRenderer*, vtkActor*, vtkOpenGLLowMemoryPolyDataMapper*) const
{
}

VTK_ABI_NAMESPACE_END
