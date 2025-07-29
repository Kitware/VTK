// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLLowMemoryVerticesAgent.h"
#include "vtkCellType.h"
#include "vtkOpenGLLowMemoryPolyDataMapper.h"
#include "vtkShaderProgram.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkOpenGLLowMemoryVerticesAgent::vtkOpenGLLowMemoryVerticesAgent() = default;

//------------------------------------------------------------------------------
vtkOpenGLLowMemoryVerticesAgent::~vtkOpenGLLowMemoryVerticesAgent() = default;

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryVerticesAgent::PreDrawInternal(
  vtkRenderer*, vtkActor*, vtkOpenGLLowMemoryPolyDataMapper* lmMapper) const
{
  lmMapper->ElementType = vtkDrawTexturedElements::ElementShape::Point;
  lmMapper->ShaderProgram->SetUniformi("cellType", VTK_VERTEX);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryVerticesAgent::PostDrawInternal(
  vtkRenderer*, vtkActor*, vtkOpenGLLowMemoryPolyDataMapper*) const
{
}

VTK_ABI_NAMESPACE_END
