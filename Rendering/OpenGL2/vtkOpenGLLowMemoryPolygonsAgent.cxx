// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLLowMemoryPolygonsAgent.h"
#include "vtkActor.h"
#include "vtkCellType.h"
#include "vtkOpenGLLowMemoryPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkShaderProgram.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkOpenGLLowMemoryPolygonsAgent::vtkOpenGLLowMemoryPolygonsAgent()
{
  // agent draws polygons as a collection of triangles.
  this->NumberOfPointsPerPrimitive = 3;
}

//------------------------------------------------------------------------------
vtkOpenGLLowMemoryPolygonsAgent::~vtkOpenGLLowMemoryPolygonsAgent() = default;

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolygonsAgent::PreDrawInternal(
  vtkRenderer*, vtkActor*, vtkOpenGLLowMemoryPolyDataMapper* lmMapper) const
{
  lmMapper->ElementType = vtkDrawTexturedElements::ElementShape::Triangle;
  lmMapper->ShaderProgram->SetUniformi("cellType", VTK_TRIANGLE);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryPolygonsAgent::PostDrawInternal(
  vtkRenderer*, vtkActor*, vtkOpenGLLowMemoryPolyDataMapper*) const
{
}

VTK_ABI_NAMESPACE_END
