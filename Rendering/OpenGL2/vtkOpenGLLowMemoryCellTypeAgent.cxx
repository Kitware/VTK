// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLLowMemoryCellTypeAgent.h"
#include "vtkCellGraphicsPrimitiveMap.h"
#include "vtkCollectionIterator.h"
#include "vtkDrawTexturedElements.h"
#include "vtkGLSLModCamera.h"
#include "vtkInformation.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLLowMemoryPolyDataMapper.h"
#include "vtkOpenGLRenderPass.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkStringToken.h"
#include "vtkTextureObject.h"
#include "vtk_glad.h"

#include <memory>

VTK_ABI_NAMESPACE_BEGIN

// Uncomment to view cell group information from `BindArraysToTextureBuffers()` and `Draw()`
// #define vtkOpenGLLowMemoryCellTypeAgent_DEBUG

namespace
{
float GetPointPickingPrimitiveSize(int type)
{
  if (type == vtkDrawTexturedElements::ElementShape::Point)
  {
    return 2;
  }
  if (type == vtkDrawTexturedElements::ElementShape::Line)
  {
    return 4;
  }
  return 6;
}
}

//------------------------------------------------------------------------------
vtkOpenGLLowMemoryCellTypeAgent::vtkOpenGLLowMemoryCellTypeAgent() = default;

//------------------------------------------------------------------------------
vtkOpenGLLowMemoryCellTypeAgent::~vtkOpenGLLowMemoryCellTypeAgent() = default;

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryCellTypeAgent::PreDraw(
  vtkRenderer* renderer, vtkActor* actor, vtkOpenGLLowMemoryPolyDataMapper* mapper) const
{
  if (!mapper)
  {
    return;
  }
  this->PreDrawInternal(renderer, actor, mapper);
  if (actor->GetProperty()->GetRepresentation() == VTK_POINTS || this->InVertexVisibilityPass)
  {
    mapper->ElementType = vtkDrawTexturedElements::ElementShape::Point;
  }
  // wacky backwards compatibility with old VTK lighting
  // soooo there are many factors that determine if a primitive is lit or not.
  // three that mix in a complex way are representation POINT, Interpolation FLAT
  // and having normals or not.
  bool needLighting = false;
  if (actor->GetProperty()->GetRepresentation() == VTK_POINTS)
  {
    needLighting =
      (actor->GetProperty()->GetInterpolation() != VTK_FLAT && mapper->HasPointNormals);
  }
  else // wireframe or surface rep
  {
    bool isTrisOrStrips = (this->NumberOfPointsPerPrimitive >= 3);
    needLighting = (isTrisOrStrips ||
      (!isTrisOrStrips && actor->GetProperty()->GetInterpolation() != VTK_FLAT &&
        mapper->HasPointNormals));
  }
  mapper->ShaderProgram->SetUniformi("enable_lights", needLighting);
  mapper->ShaderProgram->SetUniformi("vertex_pass", this->InVertexVisibilityPass);
  switch (mapper->ElementType)
  {
    case vtkDrawTexturedElements::ElementShape::Point:
      mapper->ShaderProgram->SetUniformi("primitiveSize", 1);
      break;
    case vtkDrawTexturedElements::ElementShape::Line:
      mapper->ShaderProgram->SetUniformi("primitiveSize", 2);
      break;
    case vtkDrawTexturedElements::ElementShape::Triangle:
    default:
      mapper->ShaderProgram->SetUniformi("primitiveSize", 3);
      break;
  }
  mapper->ShaderProgram->SetUniformf("pointSize",
    mapper->PointPicking ? ::GetPointPickingPrimitiveSize(mapper->ElementType)
                         : actor->GetProperty()->GetPointSize());
  mapper->vtkDrawTexturedElements::PreDraw(renderer, actor, mapper);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryCellTypeAgent::Draw(vtkRenderer* renderer, vtkActor* actor,
  vtkOpenGLLowMemoryPolyDataMapper* mapper,
  const std::vector<vtkOpenGLLowMemoryPolyDataMapper::CellGroupInformation>& cellGroups,
  std::size_t cellGroupIdx /*=0*/) const
{
  if (!mapper)
  {
    return;
  }
  if (cellGroupIdx >= cellGroups.size())
  {
    std::cerr << cellGroupIdx << " > " << cellGroups.size() << std::endl;
    return;
  }
  const auto& cellGroup = cellGroups[cellGroupIdx];
  if (!cellGroup.CanRender)
  {
    return;
  }
#ifdef vtkOpenGLLowMemoryCellTypeAgent_DEBUG
  std::cout << this << " Draw CellGroups[" << cellGroupIdx << '/' << cellGroups.size()
            << "]: " << cellGroup << '\n';
#endif
  const auto& offsets = cellGroup.Offsets;
  mapper->FirstVertexId = offsets.VertexIdOffset;
  mapper->NumberOfElements = cellGroup.NumberOfElements;
  // when rendering vertices, increase number of elements and draw 1 instance.
  if (actor->GetProperty()->GetRepresentation() == VTK_POINTS || this->InVertexVisibilityPass)
  {
    mapper->NumberOfElements *= this->NumberOfPointsPerPrimitive;
  }
  else
  {
    mapper->NumberOfElements *= this->NumberOfPseudoPrimitivesPerElement;
  }
  mapper->ShaderProgram->SetUniformi("cellIdOffset", offsets.CellIdOffset);
  mapper->ShaderProgram->SetUniformi("vertexIdOffset", offsets.VertexIdOffset);
  mapper->ShaderProgram->SetUniformi("edgeValueBufferOffset", offsets.EdgeValueBufferOffset);
  mapper->ShaderProgram->SetUniformi("pointIdOffset", offsets.PointIdOffset);
  mapper->ShaderProgram->SetUniformi("primitiveIdOffset", offsets.PrimitiveIdOffset);
  mapper->ShaderProgram->SetUniformi("usesCellMap", cellGroup.UsesCellMapBuffer);
  mapper->ShaderProgram->SetUniformi("usesEdgeValues", cellGroup.UsesEdgeValueBuffer);
  mapper->vtkDrawTexturedElements::DrawInstancedElementsImpl(renderer, actor, mapper);
}

//------------------------------------------------------------------------------
void vtkOpenGLLowMemoryCellTypeAgent::PostDraw(
  vtkRenderer* renderer, vtkActor* actor, vtkOpenGLLowMemoryPolyDataMapper* mapper) const
{
  if (!mapper)
  {
    return;
  }
  // Follow reverse order as in PreDraw.
  mapper->vtkDrawTexturedElements::PostDraw(renderer, actor, mapper);
  this->PostDrawInternal(renderer, actor, mapper);
}

VTK_ABI_NAMESPACE_END
