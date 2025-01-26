// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGraphicsPrimitiveMap.h"
#include "vtkCellArrayIterator.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

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
    const auto iter = vtk::TakeSmartPointer(mesh->GetPolys()->NewIterator());
    for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell(), ++cellIDOffset)
    {
      const vtkIdType* pts = nullptr;
      vtkIdType npts = 0;
      iter->GetCurrentCell(npts, pts);
      const int numSubTriangles = npts - 2;
      if (numSubTriangles <= 0)
      {
        continue;
      }
      double ef0 = 0;
      double ef1 = 0;
      double ef2 = 0;
      if (ef)
      {
        ef->GetTuple(pts[0], &ef0);
      }
      for (int i = 0; i < numSubTriangles; ++i)
      {
        result.PrimitiveToCell->InsertNextValue(cellIDOffset);
        result.VertexIDs->InsertNextValue(pts[0]);
        result.VertexIDs->InsertNextValue(pts[i + 1]);
        result.VertexIDs->InsertNextValue(pts[i + 2]);
        uint8_t val = npts == 3 ? 7 : i == 0 ? 3 : i == numSubTriangles - 1 ? 6 : 2;
        if (ef)
        {
          int mask = 0;
          ef->GetTuple(pts[i + 1], &ef1);
          ef->GetTuple(pts[i + 2], &ef2);
          mask = ef0 + ef1 * 2 + ef2 * 4;
          result.EdgeArray->InsertNextValue(val & mask);
        }
        else
        {
          result.EdgeArray->InsertNextValue(val);
        }
      }
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
