// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Helpers of the tests of vtkResolveHangingNodes: the partitions of a mesh, the merge of the
// partitions of an output, the checks shared by all the outputs, and mock filters.
#ifndef ResolveHangingNodesTestUtilities_h
#define ResolveHangingNodesTestUtilities_h

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
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataToUnstructuredGrid.h"
#include "vtkPolygon.h"
#include "vtkResolveHangingNodes.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <vector>

// The functions are inline, so that a test that does not use one gets no warning.
namespace
{
constexpr double EPSILON = 1e-6;

// A mesh: its area (2D) or volume (3D), the length (2D) or area (3D) of its boundary, and its
// number of input cells.
struct Domain
{
  double Measure;
  double BoundaryMeasure;
  vtkIdType NumberOfCells;
};

// The 2D 3-level mesh, over the domain [0,8] x [0,4] rotated by 30 degrees around the z axis.
constexpr Domain THREE_LEVEL_DOMAIN = { 32.0, 24.0, 8 };

// The 2D 3-level mesh is split in 2 partitions (see Split): the cells 0-3 (levels 0 and 1), and
// the cells 4-7 (level 2). The hanging point (4,2) of cell 0 is in its partition, and the hanging
// points (7,2) and (6,3) of the cells 2 and 3 are in the other one.
constexpr vtkIdType THREE_LEVEL_SPLIT = 4;

inline std::array<double, 3> Rotate(double x, double y)
{
  const double c = std::cos(vtkMath::Pi() / 6.0);
  const double s = std::sin(vtkMath::Pi() / 6.0);
  return { c * x - s * y, s * x + c * y, 0.0 };
}

// 2D 3-level mesh over the domain [0,8] x [0,4]:
// - Cell 0, level 0: [0,4] x [0,4] (area 16). Its edge x = 4 has the hanging point (4,2).
// - Cells 1-3, level 1: [4,6] x [0,2], [6,8] x [0,2], [4,6] x [2,4] (area 4). The edge y = 2 of
//   cell 2 has the hanging point (7,2), and the edge x = 6 of cell 3 has the hanging point (6,3).
// - Cells 4-7, level 2: [6,8] x [2,4] subdivided 2 x 2 (area 1).
// Cell data "CellIds" holds the cell id, and point data "Values" holds x + 10 y, after the point
// global ids "GlobalIds" if `withGlobalIds` is true.
inline vtkSmartPointer<vtkPolyData> CreateThreeLevelMesh(bool withGlobalIds = false)
{
  vtkNew<vtkPoints> points;
  points->SetDataType(VTK_DOUBLE);
  vtkNew<vtkDoubleArray> values;
  values->SetName("Values");
  vtkNew<vtkIdTypeArray> globalIds;
  globalIds->SetName("GlobalIds");
  auto add = [&](double x, double y)
  {
    values->InsertNextValue(x + 10.0 * y);
    globalIds->InsertNextValue(points->GetNumberOfPoints());
    return points->InsertNextPoint(Rotate(x, y).data());
  };
  const vtkIdType p00 = add(0, 0), p40 = add(4, 0), p44 = add(4, 4), p04 = add(0, 4);
  const vtkIdType p80 = add(8, 0), p84 = add(8, 4), p42 = add(4, 2), p60 = add(6, 0),
                  p62 = add(6, 2), p64 = add(6, 4), p82 = add(8, 2);
  const vtkIdType p72 = add(7, 2), p83 = add(8, 3), p74 = add(7, 4), p63 = add(6, 3),
                  p73 = add(7, 3);

  vtkNew<vtkCellArray> polys;
  auto quad = [&](vtkIdType a, vtkIdType b, vtkIdType c, vtkIdType d)
  {
    const vtkIdType q[4] = { a, b, c, d };
    polys->InsertNextCell(4, q);
  };
  quad(p00, p40, p44, p04);
  quad(p40, p60, p62, p42);
  quad(p60, p80, p82, p62);
  quad(p42, p62, p64, p44);
  quad(p62, p72, p73, p63);
  quad(p72, p82, p83, p73);
  quad(p63, p73, p74, p64);
  quad(p73, p83, p84, p74);

  auto pd = vtkSmartPointer<vtkPolyData>::New();
  pd->SetPoints(points);
  pd->SetPolys(polys);
  if (withGlobalIds)
  {
    pd->GetPointData()->SetGlobalIds(globalIds);
  }
  pd->GetPointData()->AddArray(values);
  vtkNew<vtkIdTypeArray> cellIds;
  cellIds->SetName("CellIds");
  for (vtkIdType i = 0; i < 8; ++i)
  {
    cellIds->InsertNextValue(i);
  }
  pd->GetCellData()->AddArray(cellIds);
  return pd;
}

// The domain of the 3D 2-level mesh: [0,4] x [0,4] x [0,2].
constexpr Domain TWO_LEVEL_DOMAIN = { 32.0, 64.0, 11 };

// The 3D 2-level mesh is split in 2 partitions (see Split): the coarse cells A, B and C, and the
// fine cells D, so that all the hanging points are in the other partition.
constexpr vtkIdType TWO_LEVEL_SPLIT = 3;

// Builds an unstructured grid of axis-aligned hexahedra given by their min corner and size. Cell
// data "CellIds" holds the cell id, and point data "Values" holds x + 10 y + 100 z, after the point
// global ids "GlobalIds" if they are requested.
class HexMeshBuilder
{
public:
  HexMeshBuilder()
  {
    this->Points->SetDataType(VTK_DOUBLE);
    this->Values->SetName("Values");
  }

  void AddHex(int x, int y, int z, int size)
  {
    const vtkIdType ids[8] = {
      this->Point(x, y, z),
      this->Point(x + size, y, z),
      this->Point(x + size, y + size, z),
      this->Point(x, y + size, z),
      this->Point(x, y, z + size),
      this->Point(x + size, y, z + size),
      this->Point(x + size, y + size, z + size),
      this->Point(x, y + size, z + size),
    };
    this->Hexes.emplace_back(ids, ids + 8);
  }

  vtkIdType Point(int x, int y, int z)
  {
    auto it = this->PointIds.find({ x, y, z });
    if (it != this->PointIds.end())
    {
      return it->second;
    }
    this->Values->InsertNextValue(x + 10.0 * y + 100.0 * z);
    const vtkIdType id = this->Points->InsertNextPoint(x, y, z);
    this->PointIds[{ x, y, z }] = id;
    return id;
  }

  vtkSmartPointer<vtkUnstructuredGrid> Build(bool withGlobalIds = false)
  {
    auto ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
    ug->AllocateExact(static_cast<vtkIdType>(this->Hexes.size()), 8);
    vtkNew<vtkIdTypeArray> cellIds;
    cellIds->SetName("CellIds");
    for (const auto& hex : this->Hexes)
    {
      cellIds->InsertNextValue(ug->InsertNextCell(VTK_HEXAHEDRON, 8, hex.data()));
    }
    ug->SetPoints(this->Points);
    if (withGlobalIds)
    {
      vtkNew<vtkIdTypeArray> globalIds;
      globalIds->SetName("GlobalIds");
      globalIds->SetNumberOfValues(this->Points->GetNumberOfPoints());
      std::iota(globalIds->GetPointer(0), globalIds->GetPointer(0) + globalIds->GetNumberOfValues(),
        static_cast<vtkIdType>(0));
      ug->GetPointData()->SetGlobalIds(globalIds);
    }
    ug->GetPointData()->AddArray(this->Values);
    ug->GetCellData()->AddArray(cellIds);
    return ug;
  }

private:
  vtkNew<vtkPoints> Points;
  vtkNew<vtkDoubleArray> Values;
  std::map<std::array<int, 3>, vtkIdType> PointIds;
  std::vector<std::vector<vtkIdType>> Hexes;
};

// 3D 2-level mesh over the domain [0,4] x [0,4] x [0,2]:
// - Cell 0 (A), level 0: [0,2] x [0,2] x [0,2]. Its face x = 2 borders the fine block D.
// - Cell 1 (B), level 0: [0,2] x [2,4] x [0,2]. It touches D only along the edge x = 2, y = 2:
//   its faces are all shared with A and C or on the boundary of the domain, so the hanging point
//   (2,2,1) on that edge is only found through A and C.
// - Cell 2 (C), level 0: [2,4] x [2,4] x [0,2]. Its face y = 2 borders D.
// - Cells 3-10 (D), level 1: [2,4] x [0,2] x [0,2] subdivided 2 x 2 x 2, cell 3 + i + 2 j + 4 k
//   being [2+i,3+i] x [j,j+1] x [k,k+1].
inline vtkSmartPointer<vtkUnstructuredGrid> CreateTwoLevelMesh()
{
  HexMeshBuilder builder;
  builder.AddHex(0, 0, 0, 2);
  builder.AddHex(0, 2, 0, 2);
  builder.AddHex(2, 2, 0, 2);
  for (int k = 0; k < 2; ++k)
  {
    for (int j = 0; j < 2; ++j)
    {
      for (int i = 0; i < 2; ++i)
      {
        builder.AddHex(2 + i, j, k, 1);
      }
    }
  }
  return builder.Build();
}

// A block of level `level` of a vtkNonOverlappingAMR: a vtkImageData of `numCells` cells (0 along z
// in 2D) of size `spacing`, from `origin`. Cell data "CellIds" holds the cell ids from
// `firstCellId`, and point data "Values" holds x + 10 y + 100 z, and "Levels" the level.
inline vtkSmartPointer<vtkImageData> CreateBlock(int level, const std::array<double, 3>& origin,
  const std::array<int, 3>& numCells, double spacing, vtkIdType firstCellId)
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
  vtkNew<vtkIdTypeArray> cellIds;
  cellIds->SetName("CellIds");
  for (vtkIdType i = 0; i < block->GetNumberOfCells(); ++i)
  {
    cellIds->InsertNextValue(firstCellId + i);
  }
  block->GetCellData()->AddArray(cellIds);
  return block;
}

inline vtkSmartPointer<vtkUnstructuredGrid> ToUnstructuredGrid(vtkPolyData* pd)
{
  vtkNew<vtkPolyDataToUnstructuredGrid> converter;
  converter->SetInputData(pd);
  converter->Update();
  return converter->GetOutput();
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

// The cells `cellIds` of `mesh`, in this order, with the points they use and the point and cell
// data, global ids included, in a dataset of the type of `mesh`, whose cells are polygons if it is
// a vtkPolyData.
inline vtkSmartPointer<vtkPointSet> ExtractCells(
  vtkPointSet* mesh, const std::vector<vtkIdType>& cellIds)
{
  vtkNew<vtkPoints> points;
  points->SetDataType(mesh->GetPoints()->GetDataType());
  auto part = vtk::TakeSmartPointer(mesh->NewInstance());
  vtkPointData* pointData = part->GetPointData();
  pointData->CopyGlobalIdsOn();
  pointData->CopyAllocate(mesh->GetPointData());
  vtkCellData* cellData = part->GetCellData();
  cellData->CopyAllocate(mesh->GetCellData());
  vtkNew<vtkCellArray> cells;
  std::vector<int> types;
  std::map<vtkIdType, vtkIdType> pointIds;
  vtkNew<vtkIdList> ids;
  for (vtkIdType i = 0; i < static_cast<vtkIdType>(cellIds.size()); ++i)
  {
    const vtkIdType c = cellIds[i];
    mesh->GetCellPoints(c, ids);
    for (vtkIdType j = 0; j < ids->GetNumberOfIds(); ++j)
    {
      const auto [it, isNew] = pointIds.emplace(ids->GetId(j), points->GetNumberOfPoints());
      if (isNew)
      {
        points->InsertNextPoint(mesh->GetPoint(ids->GetId(j)));
        pointData->CopyData(mesh->GetPointData(), ids->GetId(j), it->second);
      }
      ids->SetId(j, it->second);
    }
    cells->InsertNextCell(ids);
    types.push_back(mesh->GetCellType(c));
    cellData->CopyData(mesh->GetCellData(), c, i);
  }
  part->SetPoints(points);
  if (auto polyData = vtkPolyData::SafeDownCast(part))
  {
    polyData->SetPolys(cells);
  }
  else
  {
    vtkUnstructuredGrid::SafeDownCast(part)->SetCells(types.data(), cells);
  }
  return part;
}

// The cells [begin, end) of `mesh`, see above.
inline vtkSmartPointer<vtkPointSet> ExtractCells(vtkPointSet* mesh, vtkIdType begin, vtkIdType end)
{
  std::vector<vtkIdType> cellIds(end - begin);
  std::iota(cellIds.begin(), cellIds.end(), begin);
  return ExtractCells(mesh, cellIds);
}

// `mesh` in 2 partitions, its first `numCellsOfFirst` cells and the others, of the single
// vtkPartitionedDataSet of a vtkPartitionedDataSetCollection, so that they are resolved together.
inline vtkSmartPointer<vtkPartitionedDataSetCollection> Split(
  vtkPointSet* mesh, vtkIdType numCellsOfFirst)
{
  vtkNew<vtkPartitionedDataSet> partitions;
  partitions->SetPartition(0, ExtractCells(mesh, 0, numCellsOfFirst));
  partitions->SetPartition(1, ExtractCells(mesh, numCellsOfFirst, mesh->GetNumberOfCells()));
  auto collection = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
  collection->SetPartitionedDataSet(0, partitions);
  return collection;
}

// The partitions of `output` in one mesh, with the points they share merged.
inline vtkSmartPointer<vtkUnstructuredGrid> Merge(vtkDataObject* output)
{
  vtkNew<vtkAppendFilter> append;
  append->MergePointsOn();
  for (vtkDataSet* partition : vtkCompositeDataSet::GetDataSets(output))
  {
    append->AddInputData(partition);
  }
  append->Update();
  return append->GetOutput();
}

// Checks shared by all the outputs of a mesh of `domain`, once their partitions are merged. The
// cell data "CellIds" of the input holds the cell ids, and the point data "Values" is passed.
template <int Dim>
inline bool CheckOutput(const std::string& name, vtkUnstructuredGrid* output, bool decompose,
  vtkIdType expectedPoints, vtkIdType expectedCells, const Domain& domain)
{
  if (output->GetNumberOfPoints() != expectedPoints || output->GetNumberOfCells() != expectedCells)
  {
    std::cerr << name << ": expected " << expectedPoints << " points and " << expectedCells
              << " cells, got " << output->GetNumberOfPoints() << " and "
              << output->GetNumberOfCells() << "\n";
    return false;
  }

  // The output is conforming.
  const double boundaryMeasure = BoundaryMeasure<Dim>(output);
  if (std::abs(boundaryMeasure - domain.BoundaryMeasure) > EPSILON)
  {
    std::cerr << name << ": the boundary " << (Dim == 2 ? "length" : "area") << " is "
              << boundaryMeasure << " instead of " << domain.BoundaryMeasure
              << ", the output is not conforming\n";
    return false;
  }

  // Cell types: polygons or polyhedra with hanging points, or triangles and quads in 2D, and
  // tetrahedra and pyramids in 3D.
  for (vtkIdType c = 0; c < output->GetNumberOfCells(); ++c)
  {
    const int type = output->GetCellType(c);
    const bool isValidType = Dim == 2
      ? type == VTK_QUAD || type == (decompose ? VTK_TRIANGLE : VTK_POLYGON)
      : type == VTK_HEXAHEDRON ||
        (decompose ? (type == VTK_TETRA || type == VTK_PYRAMID) : type == VTK_POLYHEDRON);
    if (!isValidType)
    {
      std::cerr << name << ": unexpected type " << type << " for cell " << c << "\n";
      return false;
    }
  }

  // 3D: the cells are valid, in particular the tetrahedra and pyramids are not inverted.
  if (Dim == 3)
  {
    vtkNew<vtkCellValidator> validator;
    validator->SetInputData(output);
    validator->Update();
    vtkDataArray* states =
      vtkDataSet::SafeDownCast(validator->GetOutput())->GetCellData()->GetArray("ValidityState");
    for (vtkIdType c = 0; c < output->GetNumberOfCells(); ++c)
    {
      if (states->GetTuple1(c) != static_cast<short>(vtkCellValidator::State::Valid))
      {
        std::cerr << name << ": cell " << c << " of type " << output->GetCellType(c)
                  << " is not valid (state " << states->GetTuple1(c) << ")\n";
        return false;
      }
    }
  }

  // The cells (or sub-cells) of every input cell are in input order and copy its cell data.
  vtkDataArray* cellIds = output->GetCellData()->GetArray("CellIds");
  if (!cellIds || cellIds->GetTuple1(0) != 0 ||
    cellIds->GetTuple1(expectedCells - 1) != domain.NumberOfCells - 1)
  {
    std::cerr << name << ": cell data was not copied\n";
    return false;
  }
  for (vtkIdType c = 1; c < expectedCells; ++c)
  {
    const double step = cellIds->GetTuple1(c) - cellIds->GetTuple1(c - 1);
    if (step != 0 && step != 1)
    {
      std::cerr << name << ": cells are not in input order\n";
      return false;
    }
  }

  // The inverse measures are positive and cover the domain.
  const char* inverseMeasureName = Dim == 2 ? "InverseArea" : "InverseVolume";
  vtkDataArray* inverseMeasure = output->GetCellData()->GetArray(inverseMeasureName);
  if (!inverseMeasure ||
    output->GetCellData()->GetArray(Dim == 2 ? "InverseVolume" : "InverseArea"))
  {
    std::cerr << name << ": expected the " << inverseMeasureName << " cell data array only\n";
    return false;
  }
  double measure = 0.0;
  for (vtkIdType c = 0; c < output->GetNumberOfCells(); ++c)
  {
    if (inverseMeasure->GetTuple1(c) <= 0.0)
    {
      std::cerr << name << ": wrong " << inverseMeasureName << " for cell " << c << "\n";
      return false;
    }
    measure += 1.0 / inverseMeasure->GetTuple1(c);
  }
  if (std::abs(measure - domain.Measure) > EPSILON)
  {
    std::cerr << name << ": the cells cover " << (Dim == 2 ? "an area" : "a volume") << " of "
              << measure << " instead of " << domain.Measure << "\n";
    return false;
  }

  // Point data is passed.
  if (!output->GetPointData()->GetArray("Values"))
  {
    std::cerr << name << ": point data was not passed\n";
    return false;
  }
  return true;
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

// Mock subclass providing points of other processes without an MPI controller, in addition to the
// points of the other local partitions.
class vtkRemotePointsTestFilter : public vtkResolveHangingNodes
{
public:
  static vtkRemotePointsTestFilter* New() { VTK_STANDARD_NEW_BODY(vtkRemotePointsTestFilter); }
  vtkTypeMacro(vtkRemotePointsTestFilter, vtkResolveHangingNodes);

  vtkSmartPointer<vtkPolyData> Remote;

protected:
  bool UseRemotePoints() override { return true; }
  std::vector<vtkSmartPointer<vtkPolyData>> GetRemotePoints(
    const std::vector<vtkPolyData*>& boundaries,
    const std::vector<int>& partitionedDataSetIds) override
  {
    std::vector<vtkSmartPointer<vtkPolyData>> remotePoints =
      this->Superclass::GetRemotePoints(boundaries, partitionedDataSetIds);
    for (std::size_t p = 0; p < boundaries.size(); ++p)
    {
      remotePoints[p] =
        this->MergeReceivedPoints({ { 0, remotePoints[p] }, { 1, this->Remote } }, boundaries[p]);
    }
    return remotePoints;
  }
};

} // namespace

#endif
