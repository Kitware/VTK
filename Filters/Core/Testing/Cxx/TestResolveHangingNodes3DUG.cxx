// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkResolveHangingNodes on 3D vtkUnstructuredGrid meshes of hexahedra with 2 levels, whole
// or in 2 partitions of a vtkPartitionedDataSetCollection: polyhedron and decomposition modes,
// conforming and empty inputs, inverse volumes, points of other processes, the weighted
// interpolation of cell data to points, and point global ids.
#include "ResolveHangingNodesTestUtilities.h"

#include "vtkCellData.h"
#include "vtkCellDataToPointData.h"
#include "vtkCellType.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkResolveHangingNodes.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace
{
// The domain of the L-shaped mesh: [0,4] x [0,2] x [0,2] and [0,2] x [2,4] x [0,2].
constexpr Domain L_SHAPED_DOMAIN = { 24.0, 56.0, 17 };

// The domain of the stacked L-shaped mesh: [0,4] x [0,2] x [0,4] and [0,2] x [2,4] x [0,4].
constexpr Domain STACKED_L_SHAPED_DOMAIN = { 48.0, 88.0, 34 };

// The domain of the stacked T-shaped mesh: [0,6] x [0,2] x [0,4] and [2,4] x [2,4] x [0,4].
constexpr Domain STACKED_T_SHAPED_DOMAIN = { 64.0, 112.0, 50 };

// L-shaped mesh, whose coarse cell borders finer cells across 2 faces, so that its decomposition
// needs a point at its center (Category B):
// - Cell 0 (A), level 0: [0,2] x [0,2] x [0,2]. Its faces x = 2 and y = 2 border D and E, and
//   their common edge has the hanging point (2,2,1).
// - Cells 1-8 (D), level 1: [2,4] x [0,2] x [0,2] subdivided 2 x 2 x 2.
// - Cells 9-16 (E), level 1: [0,2] x [2,4] x [0,2] subdivided 2 x 2 x 2.
vtkSmartPointer<vtkUnstructuredGrid> CreateLShapedMesh(bool withGlobalIds = false)
{
  HexMeshBuilder builder;
  builder.AddHex(0, 0, 0, 2);
  for (const auto& origin : { std::array<int, 2>{ 2, 0 }, std::array<int, 2>{ 0, 2 } })
  {
    for (int k = 0; k < 2; ++k)
    {
      for (int j = 0; j < 2; ++j)
      {
        for (int i = 0; i < 2; ++i)
        {
          builder.AddHex(origin[0] + i, origin[1] + j, k, 1);
        }
      }
    }
  }
  return builder.Build(withGlobalIds);
}

// The L-shaped mesh stacked twice along z: the coarse cells A (cell 0, z in [0,2]) and A' (cell 1,
// z in [2,4]) border finer cells across their faces x = 2 and y = 2, so both are decomposed with a
// point at their center (Category B), and their common face z = 2 has 2 adjacent split edges,
// which both must decompose the same way.
vtkSmartPointer<vtkUnstructuredGrid> CreateStackedLShapedMesh()
{
  HexMeshBuilder builder;
  builder.AddHex(0, 0, 0, 2);
  builder.AddHex(0, 0, 2, 2);
  for (const auto& origin : { std::array<int, 2>{ 2, 0 }, std::array<int, 2>{ 0, 2 } })
  {
    for (int k = 0; k < 4; ++k)
    {
      for (int j = 0; j < 2; ++j)
      {
        for (int i = 0; i < 2; ++i)
        {
          builder.AddHex(origin[0] + i, origin[1] + j, k, 1);
        }
      }
    }
  }
  return builder.Build();
}

// The stacked L-shaped mesh with a third fine block: the coarse cells A (cell 0, z in [0,2]) and
// A' (cell 1, z in [2,4]) are [2,4] x [0,2], and border finer cells across their faces x = 2,
// x = 4 and y = 2, so their common face z = 2 has 3 split edges, which both must decompose the
// same way.
vtkSmartPointer<vtkUnstructuredGrid> CreateStackedTShapedMesh()
{
  HexMeshBuilder builder;
  builder.AddHex(2, 0, 0, 2);
  builder.AddHex(2, 0, 2, 2);
  for (const auto& origin :
    { std::array<int, 2>{ 0, 0 }, std::array<int, 2>{ 4, 0 }, std::array<int, 2>{ 2, 2 } })
  {
    for (int k = 0; k < 4; ++k)
    {
      for (int j = 0; j < 2; ++j)
      {
        for (int i = 0; i < 2; ++i)
        {
          builder.AddHex(origin[0] + i, origin[1] + j, k, 1);
        }
      }
    }
  }
  return builder.Build();
}

//------------------------------------------------------------------------------
// Polyhedron mode: A and C get 4 points on the edges and 1 at the center of their face bordering
// D, and B gets the point (2,2,1) on its edge. The 8 hexahedra of D are unchanged.
bool TestPolyhedronMode(vtkDataObject* input, const std::string& name)
{
  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(input);
  filter->SetDecompose(false);
  filter->SetComputeInverseMeasure(true);
  filter->Update();

  if (filter->GetNumberOfLevels() != 2)
  {
    std::cerr << name << ": expected 2 levels, got " << filter->GetNumberOfLevels() << "\n";
    return false;
  }
  if (filter->GetOutputDataObject(0)->GetDataObjectType() != input->GetDataObjectType())
  {
    std::cerr << name << ": the output should have the type of the input\n";
    return false;
  }
  auto output = Merge(filter->GetOutputDataObject(0));
  if (!CheckOutput<3>(name, output, false, 37, 11, TWO_LEVEL_DOMAIN))
  {
    return false;
  }
  const std::array<vtkIdType, 3> sizes = { 13, 9, 13 };
  for (vtkIdType c = 0; c < 3; ++c)
  {
    if (output->GetCellType(c) != VTK_POLYHEDRON || output->GetCellSize(c) != sizes[c])
    {
      std::cerr << name << ": cell " << c << " should be a polyhedron with " << sizes[c]
                << " points\n";
      return false;
    }
  }
  vtkDataArray* inverseVolume = output->GetCellData()->GetArray("InverseVolume");
  if (std::abs(inverseVolume->GetTuple1(0) - 1.0 / 8.0) > EPSILON)
  {
    std::cerr << name << ": wrong InverseVolume for a level 0 cell\n";
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
// Decomposition mode: A and C use the template of a single split face (5 pyramids and 4
// tetrahedra), and B the template of a single split edge (4 pyramids), so no point is added.
bool TestDecompositionMode(vtkDataObject* input, const std::string& name)
{
  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(input);
  filter->SetDecompose(true);
  filter->SetComputeInverseMeasure(true);
  filter->Update();

  if (filter->GetNumberOfLevels() != 2)
  {
    std::cerr << name << ": expected 2 levels, got " << filter->GetNumberOfLevels() << "\n";
    return false;
  }
  return CheckOutput<3>(
    name, Merge(filter->GetOutputDataObject(0)), true, 37, 30, TWO_LEVEL_DOMAIN);
}

//------------------------------------------------------------------------------
// The 2-level mesh in 2 partitions: the cells 5 and 9 of D ([2,3] x [1,2] x [0,2]), and the
// others. The faces x = 2 of A and y = 2 of C are split by points of their partition, on all their
// edges but their common edge, whose hanging point (2,2,1) is in the other partition. These faces
// stay on the true boundary for their unsplit edge to be searched, and the edge of B with it, so
// that A, B and C get this point and the output is conforming.
bool TestFaceSplitAcrossPartitions()
{
  auto mesh = CreateTwoLevelMesh();
  vtkNew<vtkPartitionedDataSet> partitions;
  partitions->SetPartition(0, ExtractCells(mesh, { 0, 1, 2, 3, 4, 6, 7, 8, 10 }));
  partitions->SetPartition(1, ExtractCells(mesh, { 5, 9 }));
  vtkNew<vtkPartitionedDataSetCollection> input;
  input->SetPartitionedDataSet(0, partitions);

  for (bool decompose : { false, true })
  {
    vtkNew<vtkResolveHangingNodes> filter;
    filter->SetInputData(input);
    filter->SetDecompose(decompose);
    filter->Update();

    auto output = Merge(filter->GetOutputDataObject(0));
    const vtkIdType expectedCells = decompose ? 30 : 11;
    if (output->GetNumberOfPoints() != 37 || output->GetNumberOfCells() != expectedCells ||
      std::abs(BoundaryMeasure<3>(output) - TWO_LEVEL_DOMAIN.BoundaryMeasure) > EPSILON)
    {
      std::cerr << "FaceSplitAcrossPartitions(Decompose " << decompose
                << "): the output should be conforming, with 37 points and " << expectedCells
                << " cells\n";
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
// A coarse cell bordering finer cells across 2 faces (Category B): in decomposition mode, its 6
// faces are connected to a new point at its center, whose point data is the average of the points
// of the cell. The partitions are A and D, and E: the hanging points of the face x = 2 of A are in
// its partition, and those of its face y = 2 in the other one. With `withGlobalIds`, the point data
// has global ids before the other arrays: the partitions exchange their points with their point
// data, global ids included, and the center gets the average of the other arrays, and the global
// id after the largest one of the input.
bool TestCentroidDecomposition(bool withGlobalIds)
{
  auto input = CreateLShapedMesh(withGlobalIds);
  const vtkIdType numPoints = input->GetNumberOfPoints();
  auto partitions = Split(input, 9);

  for (bool decompose : { false, true })
  {
    const std::string name =
      std::string(decompose ? "CentroidDecomposition" : "CentroidPolyhedron") +
      (withGlobalIds ? "(global ids)" : "");
    vtkNew<vtkResolveHangingNodes> filter;
    filter->SetInputData(partitions);
    filter->SetDecompose(decompose);
    filter->SetComputeInverseMeasure(true);
    filter->Update();
    auto output = Merge(filter->GetOutputDataObject(0));

    if (filter->GetNumberOfLevels() != 2)
    {
      std::cerr << name << ": expected 2 levels, got " << filter->GetNumberOfLevels() << "\n";
      return false;
    }
    // Both partitions keep their global ids.
    const auto outputs = vtkCompositeDataSet::GetDataSets(filter->GetOutputDataObject(0));
    if (withGlobalIds &&
      (outputs.size() != 2 || !outputs[0]->GetPointData()->GetGlobalIds() ||
        !outputs[1]->GetPointData()->GetGlobalIds()))
    {
      std::cerr << name << ": both partitions should keep their global ids\n";
      return false;
    }
    if (!decompose)
    {
      // A gets the 7 points of its split edges and the 2 centers of its split faces.
      if (!CheckOutput<3>(name, output, false, numPoints, 17, L_SHAPED_DOMAIN))
      {
        return false;
      }
      if (output->GetCellType(0) != VTK_POLYHEDRON || output->GetCellSize(0) != 17)
      {
        std::cerr << name << ": cell 0 should be a polyhedron with 17 points\n";
        return false;
      }
      continue;
    }

    // A is replaced by 4 pyramids per split face (2 faces), 3 tetrahedra for each of the faces
    // x = 0 and y = 0 (1 split edge, the fan from its hanging point), and 2 tetrahedra and 1
    // pyramid for each of the faces z = 0 and z = 2 (2 adjacent split edges): 20 cells, and 1 point
    // is added at its center.
    if (!CheckOutput<3>(name, output, true, numPoints + 1, 20 + 16, L_SHAPED_DOMAIN))
    {
      return false;
    }
    // The center is the only point of the output that is not a point of the input.
    vtkIdType centerId = -1;
    double center[3];
    for (vtkIdType i = 0; centerId < 0 && i < output->GetNumberOfPoints(); ++i)
    {
      output->GetPoint(i, center);
      if (FindPoint(input, { center[0], center[1], center[2] }) < 0)
      {
        centerId = i;
      }
    }
    vtkDataArray* values = output->GetPointData()->GetArray("Values");
    // The point data of the center is the average of the 17 points of A, as is its position, and
    // the values are linear in the coordinates.
    const double expected = center[0] + 10.0 * center[1] + 100.0 * center[2];
    if (centerId < 0 || std::abs(values->GetTuple1(centerId) - expected) > EPSILON ||
      center[0] <= 0.0 || center[0] >= 2.0 || center[1] <= 0.0 || center[1] >= 2.0 ||
      std::abs(center[2] - 1.0) > EPSILON)
    {
      std::cerr << name << ": wrong center point or point data\n";
      return false;
    }
    for (vtkIdType c = 0; c < 20; ++c)
    {
      vtkNew<vtkIdList> ids;
      output->GetCellPoints(c, ids);
      if (ids->IsId(centerId) < 0)
      {
        std::cerr << name << ": sub-cell " << c << " of A does not use its center point\n";
        return false;
      }
    }
    // The global id of the center, in the partition of A, is the one after the largest one of the
    // input, whose global ids are the point ids.
    const vtkIdType partitionCenterId = FindPoint(outputs[0], { center[0], center[1], center[2] });
    vtkDataArray* globalIds = outputs[0]->GetPointData()->GetGlobalIds();
    if (withGlobalIds &&
      (partitionCenterId < 0 || globalIds->GetTuple1(partitionCenterId) != numPoints))
    {
      std::cerr << name << ": the center point should get the global id " << numPoints << "\n";
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
// Two coarse cells decomposed with a point at their center share a face with 2 adjacent split
// edges, or with 3 split edges: both must decompose it the same way, which they see mirrored, for
// the output to be conforming.
// - L-shaped: each coarse cell becomes 4 pyramids per split face (2 faces), and 3 cells for each
//   of its 4 other faces (1 split edge, or 2 adjacent split edges): 20 cells.
// - T-shaped: each coarse cell becomes 4 pyramids per split face (3 faces), 2 pyramids for the
//   face y = 0 (2 opposite split edges), and 4 cells for each of the faces z = 0 and z = 2 (3 split
//   edges): 22 cells.
// The partitions are A, and the other cells, so that the 2 cells sharing the face are in different
// partitions.
bool TestSharedCentroidFace()
{
  auto lShaped = CreateStackedLShapedMesh();
  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(Split(lShaped, 1));
  filter->SetDecompose(true);
  filter->SetComputeInverseMeasure(true);
  filter->Update();
  if (!CheckOutput<3>("SharedCentroidFaceL", Merge(filter->GetOutputDataObject(0)), true,
        lShaped->GetNumberOfPoints() + 2, 2 * 20 + 32, STACKED_L_SHAPED_DOMAIN))
  {
    return false;
  }

  auto tShaped = CreateStackedTShapedMesh();
  filter->SetInputData(Split(tShaped, 1));
  filter->Update();
  return CheckOutput<3>("SharedCentroidFaceT", Merge(filter->GetOutputDataObject(0)), true,
    tShaped->GetNumberOfPoints() + 2, 2 * 22 + 48, STACKED_T_SHAPED_DOMAIN);
}

// Conforming mesh: [0,2] x [0,2] x [0,2] and [2,4] x [0,2] x [0,2].
vtkSmartPointer<vtkUnstructuredGrid> CreateConformingMesh()
{
  HexMeshBuilder builder;
  builder.AddHex(0, 0, 0, 2);
  builder.AddHex(2, 0, 0, 2);
  return builder.Build();
}

//------------------------------------------------------------------------------
// A conforming mesh, with a cell in each partition, is copied unchanged, with 1 level: the points
// that a partition receives from the other one are corners of its cell.
bool TestConformingInput()
{
  auto input = Split(CreateConformingMesh(), 1);
  for (bool decompose : { false, true })
  {
    vtkNew<vtkResolveHangingNodes> filter;
    filter->SetInputData(input);
    filter->SetDecompose(decompose);
    filter->Update();
    auto output = Merge(filter->GetOutputDataObject(0));
    if (filter->GetNumberOfLevels() != 1 || output->GetNumberOfPoints() != 12 ||
      output->GetNumberOfCells() != 2 || output->GetCellType(0) != VTK_HEXAHEDRON ||
      output->GetCellType(1) != VTK_HEXAHEDRON)
    {
      std::cerr << "ConformingInput: the mesh should be unchanged\n";
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
// An empty vtkUnstructuredGrid gives an empty vtkUnstructuredGrid.
bool TestEmptyInput()
{
  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(vtkSmartPointer<vtkUnstructuredGrid>::New());
  filter->Update();
  auto output = vtkUnstructuredGrid::SafeDownCast(filter->GetOutputDataObject(0));
  if (!output || output->GetNumberOfCells() != 0)
  {
    std::cerr << "EmptyInput: the output should be an empty vtkUnstructuredGrid\n";
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
// No inverse volume is computed by default.
bool TestNoInverseMeasure()
{
  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(Split(CreateTwoLevelMesh(), TWO_LEVEL_SPLIT));
  filter->Update();
  for (vtkDataSet* output : vtkCompositeDataSet::GetDataSets(filter->GetOutputDataObject(0)))
  {
    if (output->GetCellData()->GetArray("InverseVolume"))
    {
      std::cerr << "NoInverseMeasure: InverseVolume should not be computed\n";
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
// Points of other processes: a point on the true boundary of a cell is inserted with its point
// data, while points away from the boundary and duplicates are ignored. The point is the middle of
// the edge x = 0, y = 0 of A, as if a finer neighbor were there.
bool TestRemotePoints()
{
  auto input = CreateTwoLevelMesh();
  vtkNew<vtkPoints> remotePoints;
  remotePoints->SetDataType(VTK_DOUBLE);
  vtkNew<vtkDoubleArray> remoteValues;
  remoteValues->SetName("Values");
  auto addRemote = [&](double x, double y, double z)
  {
    remotePoints->InsertNextPoint(x, y, z);
    remoteValues->InsertNextValue(x + 10.0 * y + 100.0 * z);
  };
  addRemote(0, 0, 1);    // on the edge x = 0, y = 0 of A
  addRemote(10, 10, 10); // far away
  addRemote(0, 0, 1);    // duplicate
  addRemote(1, 1, 1);    // inside A
  vtkNew<vtkPolyData> remote;
  remote->SetPoints(remotePoints);
  remote->GetPointData()->AddArray(remoteValues);

  vtkNew<vtkRemotePointsTestFilter> filter;
  filter->Remote = remote;
  filter->SetInputData(Split(input, TWO_LEVEL_SPLIT));
  filter->SetDecompose(false);
  filter->SetComputeInverseMeasure(true);
  filter->Update();
  auto output = Merge(filter->GetOutputDataObject(0));

  const vtkIdType numPoints = input->GetNumberOfPoints();
  if (!CheckOutput<3>("RemotePoints", output, false, numPoints + 1, 11, TWO_LEVEL_DOMAIN) ||
    output->GetCellSize(0) != 14)
  {
    std::cerr << "RemotePoints: expected 1 more point, inserted in A\n";
    return false;
  }
  const vtkIdType id = FindPoint(output, { 0, 0, 1 });
  vtkDataArray* values = output->GetPointData()->GetArray("Values");
  if (id < 0 || !values || values->GetTuple1(id) != 100.0)
  {
    std::cerr << "RemotePoints: the remote point or its data is wrong\n";
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
// The inverse volumes weight the interpolation of cell data to points, so that the value of a
// point does not depend on how many finer cells use it. The partitions are merged first, so that a
// point gets the cell data of all the cells that use it.
bool TestCellDataToPointData()
{
  auto input = CreateTwoLevelMesh();
  vtkNew<vtkDoubleArray> temperature;
  temperature->SetName("Temperature");
  for (vtkIdType i = 0; i < 11; ++i)
  {
    temperature->InsertNextValue(100.0 * (i + 1));
  }
  input->GetCellData()->AddArray(temperature);

  vtkNew<vtkResolveHangingNodes> resolver;
  resolver->SetInputData(Split(input, TWO_LEVEL_SPLIT));
  resolver->SetDecompose(false);
  resolver->SetComputeInverseMeasure(true);
  resolver->Update();

  vtkNew<vtkCellDataToPointData> cellToPoint;
  cellToPoint->SetInputData(Merge(resolver->GetOutputDataObject(0)));
  cellToPoint->SetEnableWeightedAverage(true);
  cellToPoint->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "InverseVolume");
  cellToPoint->Update();

  auto output = vtkDataSet::SafeDownCast(cellToPoint->GetOutput());
  vtkDataArray* pointTemperature = output->GetPointData()->GetArray("Temperature");
  if (!pointTemperature)
  {
    std::cerr << "CellDataToPointData: missing point data\n";
    return false;
  }
  // The hanging point (2,1,1) at the center of the face x = 2 of A is used by A (volume 8) and by
  // the cells 3, 5, 7 and 9 of D (volume 1).
  const double expected = (100.0 / 8 + 400.0 + 600.0 + 800.0 + 1000.0) / (1.0 / 8 + 4.0);
  const vtkIdType hangingPoint = FindPoint(output, { 2, 1, 1 });
  if (hangingPoint < 0 || std::abs(pointTemperature->GetTuple1(hangingPoint) - expected) > EPSILON)
  {
    std::cerr << "CellDataToPointData: wrong value at the hanging point\n";
    return false;
  }
  return true;
}
} // namespace

int TestResolveHangingNodes3DUG(int, char*[])
{
  if (BoundaryMeasure<3>(CreateTwoLevelMesh()) < TWO_LEVEL_DOMAIN.BoundaryMeasure + 1.0 ||
    BoundaryMeasure<3>(CreateLShapedMesh()) < L_SHAPED_DOMAIN.BoundaryMeasure + 1.0 ||
    BoundaryMeasure<3>(CreateStackedLShapedMesh()) <
      STACKED_L_SHAPED_DOMAIN.BoundaryMeasure + 1.0 ||
    BoundaryMeasure<3>(CreateStackedTShapedMesh()) < STACKED_T_SHAPED_DOMAIN.BoundaryMeasure + 1.0)
  {
    std::cerr << "The inputs should have hanging nodes\n";
    return EXIT_FAILURE;
  }
  auto twoLevelMesh = CreateTwoLevelMesh();
  const std::map<std::string, vtkSmartPointer<vtkDataObject>> inputs = {
    { "vtkUnstructuredGrid", twoLevelMesh },
    { "vtkUnstructuredGrid partitions", Split(twoLevelMesh, TWO_LEVEL_SPLIT) },
  };
  for (const auto& [inputName, input] : inputs)
  {
    if (!TestPolyhedronMode(input, "PolyhedronMode(" + inputName + ")") ||
      !TestDecompositionMode(input, "DecompositionMode(" + inputName + ")"))
    {
      return EXIT_FAILURE;
    }
  }
  if (!TestFaceSplitAcrossPartitions() || !TestCentroidDecomposition(false) ||
    !TestCentroidDecomposition(true) || !TestSharedCentroidFace() || !TestConformingInput() ||
    !TestEmptyInput() || !TestNoInverseMeasure() || !TestRemotePoints() ||
    !TestCellDataToPointData())
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
