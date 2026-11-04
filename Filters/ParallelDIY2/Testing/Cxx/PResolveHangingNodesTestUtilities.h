// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Helpers of the tests of vtkPResolveHangingNodes: the partitions of the 2D and 3D meshes, the
// blocks of a vtkNonOverlappingAMR, the check of the output gathered from all the processes, and a
// mock filter.
#ifndef PResolveHangingNodesTestUtilities_h
#define PResolveHangingNodesTestUtilities_h

#include "vtkAppendFilter.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCellValidator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFeatureEdges.h"
#include "vtkGeometryFilter.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPResolveHangingNodes.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// The functions are inline, so that a test that does not use one gets no warning.
namespace
{
constexpr double EPSILON = 1e-6;

// A block of level `level` of a vtkNonOverlappingAMR: a vtkImageData of `numCells` cells (0 along z
// in 2D) of size `spacing`, from `origin`. Point data "Values" holds x + 10 y + 100 z, and "Levels"
// the level.
inline vtkSmartPointer<vtkImageData> CreateBlock(int level, const std::array<double, 3>& origin,
  const std::array<int, 3>& numCells, double spacing)
{
  auto block = vtkSmartPointer<vtkImageData>::New();
  block->SetOrigin(origin.data());
  block->SetSpacing(spacing, spacing, spacing);
  block->SetDimensions(numCells[0] + 1, numCells[1] + 1, numCells[2] + 1);
  vtkNew<vtkDoubleArray> values;
  values->SetName("Values");
  vtkNew<vtkIntArray> levels;
  levels->SetName("Levels");
  for (vtkIdType i = 0; i < block->GetNumberOfPoints(); ++i)
  {
    const double* p = block->GetPoint(i);
    values->InsertNextValue(p[0] + 10.0 * p[1] + 100.0 * p[2]);
    levels->InsertNextValue(level);
  }
  block->GetPointData()->AddArray(values);
  block->GetPointData()->AddArray(levels);
  return block;
}

// 2D 3-level mesh over the domain [0,8] x [0,4], with a quad in each of the 8 partitions, the
// partitions 2 rank and 2 rank + 1 being on the process rank:
// - Partition 0: level 0 quad [0,4] x [0,4] (area 16). Its edge x = 4 has the hanging point (4,2)
//   of the partitions 1, on the same process, and 3.
// - Partitions 1, 2 and 3: level 1 quads [4,6] x [0,2], [6,8] x [0,2] and [4,6] x [2,4] (area 4).
//   The edge y = 2 of partition 2 has the hanging point (7,2) of the partitions 4 and 5, and the
//   edge x = 6 of partition 3 has the hanging point (6,3) of the partitions 4 and 6.
// - Partitions 4 to 7: level 2 quads [6,8] x [2,4] subdivided 2 x 2 (area 1), by rows.
// Each quad [x0,x1] x [y0,y1] is { x0, y0, x1, y1 }.
const std::array<std::array<double, 4>, 8> QUADS = { {
  { 0, 0, 4, 4 },
  { 4, 0, 6, 2 },
  { 6, 0, 8, 2 },
  { 4, 2, 6, 4 },
  { 6, 2, 7, 3 },
  { 7, 2, 8, 3 },
  { 6, 3, 7, 4 },
  { 7, 3, 8, 4 },
} };

// The quad of a partition. Point data "Values" holds x + 10 y + 100 partition, to check where a
// point comes from: of the copies of a point received from several partitions, the one of the
// first partition is used.
inline vtkSmartPointer<vtkPolyData> CreateQuadPartition(int partition)
{
  vtkNew<vtkPoints> points;
  points->SetDataType(VTK_DOUBLE);
  vtkNew<vtkDoubleArray> values;
  values->SetName("Values");
  auto add = [&](double x, double y)
  {
    values->InsertNextValue(x + 10.0 * y + 100.0 * partition);
    return points->InsertNextPoint(x, y, 0.0);
  };
  const auto& [x0, y0, x1, y1] = QUADS[partition];
  vtkNew<vtkCellArray> polys;
  const vtkIdType quad[4] = { add(x0, y0), add(x1, y0), add(x1, y1), add(x0, y1) };
  polys->InsertNextCell(4, quad);

  auto pd = vtkSmartPointer<vtkPolyData>::New();
  pd->SetPoints(points);
  pd->SetPolys(polys);
  pd->GetPointData()->AddArray(values);
  return pd;
}

// 3D 2-level mesh over the domain [0,4] x [0,4] x [0,2], in 8 partitions, the partitions 2 rank and
// 2 rank + 1 being on the process rank:
// - Partition 0 (A): level 0 hexahedron [0,2] x [0,2] x [0,2]. Its face x = 2 borders D.
// - Partition 2 (B): level 0 hexahedron [0,2] x [2,4] x [0,2]. It touches D only along the edge
//   x = 2, y = 2, so it only gets the hanging point (2,2,1) from the partitions 4 and 6.
// - Partition 3 (C): level 0 hexahedron [2,4] x [2,4] x [0,2], on the process of B. Its face
//   y = 2 borders D.
// - The other partitions (D): level 1 hexahedra [2,4] x [0,2] x [0,2] subdivided 2 x 2 x 2. The
//   partitions 1, 4 and 5 have the rows of 2 hexahedra [2,4] x [j,j+1] x [k,k+1] for (j, k) =
//   (0, 0), (1, 0) and (0, 1), and the partitions 6 and 7 the hexahedra [2,3] x [1,2] x [1,2] and
//   [3,4] x [1,2] x [1,2].
// Point data "Values" holds x + 10 y + 100 z + 1000 partition, to check where a point comes from:
// of the copies of a point received from several partitions, the one of the first partition is
// used.
inline vtkSmartPointer<vtkUnstructuredGrid> CreateHexPartition(int partition)
{
  vtkNew<vtkPoints> points;
  points->SetDataType(VTK_DOUBLE);
  vtkNew<vtkDoubleArray> values;
  values->SetName("Values");
  std::map<std::array<int, 3>, vtkIdType> pointIds;
  auto point = [&](int x, int y, int z)
  {
    auto it = pointIds.find({ x, y, z });
    if (it != pointIds.end())
    {
      return it->second;
    }
    values->InsertNextValue(x + 10.0 * y + 100.0 * z + 1000.0 * partition);
    const vtkIdType id = points->InsertNextPoint(x, y, z);
    pointIds[{ x, y, z }] = id;
    return id;
  };

  auto ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
  ug->AllocateExact(8, 64);
  auto hex = [&](int x, int y, int z, int size)
  {
    const vtkIdType ids[8] = { point(x, y, z), point(x + size, y, z), point(x + size, y + size, z),
      point(x, y + size, z), point(x, y, z + size), point(x + size, y, z + size),
      point(x + size, y + size, z + size), point(x, y + size, z + size) };
    ug->InsertNextCell(VTK_HEXAHEDRON, 8, ids);
  };

  if (partition == 0)
  {
    hex(0, 0, 0, 2);
  }
  else if (partition == 2)
  {
    hex(0, 2, 0, 2);
  }
  else if (partition == 3)
  {
    hex(2, 2, 0, 2);
  }
  else if (partition >= 6)
  {
    hex(partition - 4, 1, 1, 1);
  }
  else
  {
    const int j = partition == 4 ? 1 : 0;
    const int k = partition == 5 ? 1 : 0;
    for (int i = 0; i < 2; ++i)
    {
      hex(2 + i, j, k, 1);
    }
  }
  ug->SetPoints(points);
  ug->GetPointData()->AddArray(values);
  return ug;
}

inline vtkIdType FindPoint(vtkDataSet* ds, const std::array<double, 3>& p)
{
  for (vtkIdType i = 0; i < ds->GetNumberOfPoints(); ++i)
  {
    if (vtkMath::Distance2BetweenPoints(p.data(), ds->GetPoint(i)) < EPSILON)
    {
      return i;
    }
  }
  return -1;
}

// Total length (2D) or area (3D) of the edges or faces used by a single cell. It is the boundary
// measure of the domain if and only if the mesh is conforming: a remaining hanging node leaves both
// sides of an interface unmatched.
template <int Dim>
inline double BoundaryMeasure(vtkDataSet* ds)
{
  vtkNew<vtkGeometryFilter> geometry;
  geometry->SetInputData(ds);
  geometry->Update();
  vtkSmartPointer<vtkPolyData> boundary = geometry->GetOutput();
  if constexpr (Dim == 2)
  {
    vtkNew<vtkFeatureEdges> edges;
    edges->SetInputData(boundary);
    edges->BoundaryEdgesOn();
    edges->FeatureEdgesOff();
    edges->NonManifoldEdgesOff();
    edges->ManifoldEdgesOff();
    edges->Update();
    boundary = edges->GetOutput();
  }
  double measure = 0.0;
  vtkNew<vtkIdList> ids;
  for (vtkIdType c = 0; c < boundary->GetNumberOfCells(); ++c)
  {
    boundary->GetCellPoints(c, ids);
    if constexpr (Dim == 2)
    {
      double p0[3], p1[3];
      boundary->GetPoint(ids->GetId(0), p0);
      boundary->GetPoint(ids->GetId(1), p1);
      measure += std::sqrt(vtkMath::Distance2BetweenPoints(p0, p1));
    }
    else
    {
      double normal[3];
      measure += vtkPolygon::ComputeArea(
        boundary->GetPoints(), ids->GetNumberOfIds(), ids->GetPointer(0), normal);
    }
  }
  return measure;
}

// Gather the partitions of `output` on process 0, merge the points they share, and check that the
// whole mesh is conforming, valid in 3D, and covers the domain, of area (2D) or volume (3D)
// `expectedMeasure` and of boundary length (2D) or area (3D) `expectedBoundaryMeasure`.
template <int Dim>
inline bool CheckGlobalOutput(const std::string& name, vtkMultiProcessController* controller,
  vtkDataObject* output, double expectedMeasure, double expectedBoundaryMeasure)
{
  // The partitions of this process, in one dataset to gather, empty without partitions (since
  // vtkAppendFilter needs an input)
  auto local = vtkSmartPointer<vtkUnstructuredGrid>::New();
  const auto partitions = vtkCompositeDataSet::GetDataSets(output);
  if (!partitions.empty())
  {
    vtkNew<vtkAppendFilter> localAppend;
    for (vtkDataSet* partition : partitions)
    {
      localAppend->AddInputData(partition);
    }
    localAppend->Update();
    local = localAppend->GetOutput();
  }
  std::vector<vtkSmartPointer<vtkDataObject>> outputs;
  controller->Gather(local, outputs, 0);
  if (controller->GetLocalProcessId() != 0)
  {
    return true;
  }
  vtkNew<vtkAppendFilter> append;
  append->MergePointsOn();
  for (const auto& partition : outputs)
  {
    append->AddInputData(partition);
  }
  append->Update();
  vtkUnstructuredGrid* mesh = append->GetOutput();

  const double boundaryMeasure = BoundaryMeasure<Dim>(mesh);
  if (std::abs(boundaryMeasure - expectedBoundaryMeasure) > EPSILON)
  {
    std::cerr << name << ": the boundary " << (Dim == 2 ? "length" : "area") << " is "
              << boundaryMeasure << " instead of " << expectedBoundaryMeasure
              << ", the output is not conforming\n";
    return false;
  }

  if (Dim == 3)
  {
    vtkNew<vtkCellValidator> validator;
    validator->SetInputData(mesh);
    validator->Update();
    vtkDataArray* states =
      vtkDataSet::SafeDownCast(validator->GetOutput())->GetCellData()->GetArray("ValidityState");
    for (vtkIdType c = 0; c < mesh->GetNumberOfCells(); ++c)
    {
      if (states->GetTuple1(c) != static_cast<short>(vtkCellValidator::State::Valid))
      {
        std::cerr << name << ": cell " << c << " of type " << mesh->GetCellType(c)
                  << " is not valid\n";
        return false;
      }
    }
  }

  vtkDataArray* inverseMeasure =
    mesh->GetCellData()->GetArray(Dim == 2 ? "InverseArea" : "InverseVolume");
  double measure = 0.0;
  for (vtkIdType c = 0; inverseMeasure && c < mesh->GetNumberOfCells(); ++c)
  {
    measure += 1.0 / inverseMeasure->GetTuple1(c);
  }
  if (std::abs(measure - expectedMeasure) > EPSILON)
  {
    std::cerr << name << ": the cells cover " << (Dim == 2 ? "an area" : "a volume") << " of "
              << measure << " instead of " << expectedMeasure << "\n";
    return false;
  }
  return true;
}

// Mock subclass recording the partitionedDataSetIds of the local blocks of a vtkNonOverlappingAMR,
// their levels, and checking that a block only receives the points of the blocks of the other
// levels, here or on other processes, whose point data "Levels" holds their level.
class vtkPLevelsTestFilter : public vtkPResolveHangingNodes
{
public:
  static vtkPLevelsTestFilter* New() { VTK_STANDARD_NEW_BODY(vtkPLevelsTestFilter); }
  vtkTypeMacro(vtkPLevelsTestFilter, vtkPResolveHangingNodes);

  std::vector<int> PartitionedDataSetIds;
  bool ReceivedOnlyOtherLevels = true;

protected:
  std::vector<vtkSmartPointer<vtkPolyData>> GetRemotePoints(
    const std::vector<vtkPolyData*>& boundaries,
    const std::vector<int>& partitionedDataSetIds) override
  {
    this->PartitionedDataSetIds = partitionedDataSetIds;
    std::vector<vtkSmartPointer<vtkPolyData>> remotePoints =
      this->Superclass::GetRemotePoints(boundaries, partitionedDataSetIds);
    for (std::size_t p = 0; p < remotePoints.size(); ++p)
    {
      vtkDataArray* levels =
        remotePoints[p] ? remotePoints[p]->GetPointData()->GetArray("Levels") : nullptr;
      for (vtkIdType i = 0; levels && i < levels->GetNumberOfTuples(); ++i)
      {
        if (levels->GetTuple1(i) == partitionedDataSetIds[p])
        {
          this->ReceivedOnlyOtherLevels = false;
        }
      }
    }
    return remotePoints;
  }
};
} // namespace

#endif
