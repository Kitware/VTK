// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGraphicsPrimitiveMap.h"
#include "vtkCellArrayIterator.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h" // for the shared ear-clip triangulation

#include <utility>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGraphicsPrimitiveMap);

//------------------------------------------------------------------------------
vtkCellGraphicsPrimitiveMap::vtkCellGraphicsPrimitiveMap() = default;

//------------------------------------------------------------------------------
vtkCellGraphicsPrimitiveMap::~vtkCellGraphicsPrimitiveMap() = default;

//------------------------------------------------------------------------------
void vtkCellGraphicsPrimitiveMap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkCellGraphicsPrimitiveMap::PrimitiveDescriptor vtkCellGraphicsPrimitiveMap::ProcessVertices(
  vtkPolyData* mesh)
{
  PrimitiveDescriptor result;
  if (!mesh || !mesh->GetNumberOfVerts())
  {
    return result;
  }
  vtkIdType cellIDOffset = 0;
  result.PrimitiveSize = 1;
  if (mesh->GetVerts()->GetMaxCellSize() > 1)
  {
    vtkDebugWithObjectMacro(mesh, << "Might run out of memory because there are poly vertices.");
    // tessellate polyverts into vertices only when there are polyverts with vertex count > 1
    // incurs extra memory, so warn about it.
    result.VertexIDs = vtkSmartPointer<vtkTypeInt32Array>::New();
    result.VertexIDs->SetNumberOfComponents(1);
    result.PrimitiveToCell = vtkSmartPointer<vtkTypeInt32Array>::New();
    result.PrimitiveToCell->SetNumberOfComponents(1);
    const auto iter = vtk::TakeSmartPointer(mesh->GetVerts()->NewIterator());
    for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell(), ++cellIDOffset)
    {
      const vtkIdType* pts = nullptr;
      vtkIdType npts = 0;
      iter->GetCurrentCell(npts, pts);
      for (int i = 0; i < npts; ++i)
      {
        result.PrimitiveToCell->InsertNextValue(cellIDOffset);
        result.VertexIDs->InsertNextValue(pts[i]);
      }
    }
  }
  else
  {
    result.VertexIDs = vtkSmartPointer<vtkTypeInt32Array>::New();
    result.VertexIDs->ShallowCopy(mesh->GetVerts()->GetConnectivityArray());
    result.LocalCellIdOffset = cellIDOffset;
  }
  return result;
}

//------------------------------------------------------------------------------
vtkCellGraphicsPrimitiveMap::PrimitiveDescriptor vtkCellGraphicsPrimitiveMap::ProcessLines(
  vtkPolyData* mesh)
{
  PrimitiveDescriptor result;
  if (!mesh || !mesh->GetNumberOfLines())
  {
    return result;
  }
  vtkIdType cellIDOffset = mesh->GetNumberOfVerts();
  result.PrimitiveSize = 2;
  if (mesh->GetLines()->GetMaxCellSize() > 2)
  {
    // tessellate polylines into line segments only when there are polylines with vertex count > 2
    // incurs extra memory, so warn about it.
    vtkDebugWithObjectMacro(mesh, << "Might run out of memory because there are polylines.");
    result.VertexIDs = vtkSmartPointer<vtkTypeInt32Array>::New();
    result.VertexIDs->SetNumberOfComponents(1);
    result.PrimitiveToCell = vtkSmartPointer<vtkTypeInt32Array>::New();
    result.PrimitiveToCell->SetNumberOfComponents(1);
    const auto iter = vtk::TakeSmartPointer(mesh->GetLines()->NewIterator());
    for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell(), ++cellIDOffset)
    {
      const vtkIdType* pts = nullptr;
      vtkIdType npts = 0;
      iter->GetCurrentCell(npts, pts);
      const int numSubLines = npts - 1;
      for (int i = 0; i < numSubLines; ++i)
      {
        result.PrimitiveToCell->InsertNextValue(cellIDOffset);
        result.VertexIDs->InsertNextValue(pts[i]);
        result.VertexIDs->InsertNextValue(pts[i + 1]);
      }
    }
  }
  else
  {
    result.VertexIDs = vtkSmartPointer<vtkTypeInt32Array>::New();
    result.VertexIDs->ShallowCopy(mesh->GetLines()->GetConnectivityArray());
    result.LocalCellIdOffset = cellIDOffset;
  }
  return result;
}

namespace
{
// The ear-clip triangulation is shared with vtkOpenGLIndexBufferObject via
// vtkPolygon::EarClipPolygon3D. That implementation is templated on the point/cell
// access mechanism for zero-copy reads; here the polygon comes as a vtkPoints
// plus a vtkIdType* id list, so present those through lightweight adapters with
// the same points[cell[k]] -> indexable-point semantics. The adapters are
// trivially inlined, so reading a coordinate costs exactly one
// vtkPoints::GetPoint, identical to the previous open-coded version.
struct PointAccessor
{
  vtkPoints* Points;
  struct Pt3
  {
    double X[3];
    double operator[](int i) const { return this->X[i]; }
  };
  Pt3 operator[](vtkIdType id) const
  {
    Pt3 p;
    this->Points->GetPoint(id, p.X);
    return p;
  }
};

// Forwarding overload preserving the (vtkPoints*, const vtkIdType*, npts, ...)
// call signature used below and previously by this translation unit.
template <typename EmitFn>
inline void EarClipPolygon3D(vtkPoints* points, const vtkIdType* pts, int npts,
  std::vector<int>& prevBuf, std::vector<int>& nextBuf, std::vector<int>& ring, EmitFn&& emit)
{
  PointAccessor accessor{ points };
  vtkPolygon::EarClipPolygon3D(
    accessor, pts, npts, prevBuf, nextBuf, ring, std::forward<EmitFn>(emit));
}
} // anonymous namespace

//------------------------------------------------------------------------------
vtkCellGraphicsPrimitiveMap::PrimitiveDescriptor vtkCellGraphicsPrimitiveMap::ProcessPolygons(
  vtkPolyData* mesh)
{
  PrimitiveDescriptor result;
  if (!mesh || !mesh->GetNumberOfPolys())
  {
    return result;
  }
  vtkIdType cellIDOffset = mesh->GetNumberOfVerts() + mesh->GetNumberOfLines();
  result.PrimitiveSize = 3;
  vtkDataArray* ef = mesh->GetPointData()->GetAttribute(vtkDataSetAttributes::EDGEFLAG);
  if (mesh->GetPolys()->GetMaxCellSize() > 3)
  {
    // tessellate polygons into triangles only when there are polygons with vertex count > 3
    // incurs extra memory, so warn about it.
    vtkDebugWithObjectMacro(
      mesh, << "Might run out of memory because there are polygons with greater than 3 points.");
    result.VertexIDs = vtkSmartPointer<vtkTypeInt32Array>::New();
    result.VertexIDs->SetNumberOfComponents(1);
    result.PrimitiveToCell = vtkSmartPointer<vtkTypeInt32Array>::New();
    result.PrimitiveToCell->SetNumberOfComponents(1);
    result.EdgeArray = vtkSmartPointer<vtkTypeUInt8Array>::New();
    result.EdgeArray->SetNumberOfComponents(1);
    vtkPoints* points = mesh->GetPoints();
    // Scratch buffers reused across cells by the ear-clip triangulator.
    std::vector<int> earPrev;
    std::vector<int> earNext;
    std::vector<int> earRing;
    const auto iter = vtk::TakeSmartPointer(mesh->GetPolys()->NewIterator());
    for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell(), ++cellIDOffset)
    {
      const vtkIdType* pts = nullptr;
      vtkIdType npts = 0;
      iter->GetCurrentCell(npts, pts);
      if (npts - 2 <= 0)
      {
        continue;
      }
      // Triangulate by ear clipping (matching vtkOpenGLIndexBufferObject) so
      // non-convex polygons - e.g. iso-polygons from contouring/clipping - are
      // tessellated correctly rather than fanned from vertex 0, which produces
      // overlapping or out-of-polygon triangles. boundaryMask gives the polygon-
      // boundary edges of each emitted triangle; combined with the per-vertex
      // edge flags it reproduces the historical edge-visibility values.
      EarClipPolygon3D(points, pts, static_cast<int>(npts), earPrev, earNext, earRing,
        [&](int a, int b, int c, int boundaryMask)
        {
          const vtkIdType idA = pts[a];
          const vtkIdType idB = pts[b];
          const vtkIdType idC = pts[c];
          result.PrimitiveToCell->InsertNextValue(cellIDOffset);
          result.VertexIDs->InsertNextValue(idA);
          result.VertexIDs->InsertNextValue(idB);
          result.VertexIDs->InsertNextValue(idC);
          if (ef)
          {
            double ef0 = 0, ef1 = 0, ef2 = 0;
            ef->GetTuple(idA, &ef0);
            ef->GetTuple(idB, &ef1);
            ef->GetTuple(idC, &ef2);
            const int mask =
              static_cast<int>(ef0) + static_cast<int>(ef1) * 2 + static_cast<int>(ef2) * 4;
            result.EdgeArray->InsertNextValue(static_cast<uint8_t>(boundaryMask & mask));
          }
          else
          {
            result.EdgeArray->InsertNextValue(static_cast<uint8_t>(boundaryMask));
          }
        });
    }
  }
  else
  {
    result.VertexIDs = vtkSmartPointer<vtkTypeInt32Array>::New();
    result.VertexIDs->ShallowCopy(mesh->GetPolys()->GetConnectivityArray());
    result.LocalCellIdOffset = cellIDOffset;
  }
  return result;
}

//------------------------------------------------------------------------------
vtkCellGraphicsPrimitiveMap::PrimitiveDescriptor vtkCellGraphicsPrimitiveMap::ProcessStrips(
  vtkPolyData* mesh)
{
  PrimitiveDescriptor result;
  if (!mesh || !mesh->GetNumberOfStrips())
  {
    return result;
  }
  vtkIdType cellIDOffset =
    mesh->GetNumberOfVerts() + mesh->GetNumberOfLines() + mesh->GetNumberOfPolys();
  result.PrimitiveSize = 3;
  result.VertexIDs = vtkSmartPointer<vtkTypeInt32Array>::New();
  result.VertexIDs->SetNumberOfComponents(1);
  result.PrimitiveToCell = vtkSmartPointer<vtkTypeInt32Array>::New();
  result.PrimitiveToCell->SetNumberOfComponents(1);
  const auto iter = vtk::TakeSmartPointer(mesh->GetStrips()->NewIterator());
  for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell(), ++cellIDOffset)
  {
    const vtkIdType* pts = nullptr;
    vtkIdType npts = 0;
    iter->GetCurrentCell(npts, pts);
    int p1, p2, p3, i;
    p1 = pts[0];
    p2 = pts[1];
    for (i = 0; i < (npts - 2); i++)
    {
      p3 = pts[i + 2];
      if ((i % 2)) // flip ordering to preserve consistency
      {
        result.PrimitiveToCell->InsertNextValue(cellIDOffset);
        result.VertexIDs->InsertNextValue(p2);
        result.VertexIDs->InsertNextValue(p1);
        result.VertexIDs->InsertNextValue(p3);
      }
      else
      {
        result.PrimitiveToCell->InsertNextValue(cellIDOffset);
        result.VertexIDs->InsertNextValue(p1);
        result.VertexIDs->InsertNextValue(p2);
        result.VertexIDs->InsertNextValue(p3);
      }
      p1 = p2;
      p2 = p3;
    }
  }
  return result;
}

VTK_ABI_NAMESPACE_END
