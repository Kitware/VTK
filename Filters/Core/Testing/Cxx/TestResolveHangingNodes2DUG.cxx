// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkResolveHangingNodes on a 2D vtkPolyData or vtkUnstructuredGrid mesh of quads with 3
// levels, whole or in 2 partitions of a vtkPartitionedDataSetCollection: polygon and decomposition
// modes, conforming and empty inputs, inverse areas, points of other processes, the weighted
// interpolation of cell data to points, and point global ids.
#include "ResolveHangingNodesTestUtilities.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellDataToPointData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
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
//------------------------------------------------------------------------------
// Polygon mode: cells 0, 2 and 3 become 5-gons, the others are unchanged.
bool TestPolygonMode(vtkDataObject* input, const std::string& name)
{
  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(input);
  filter->SetDecompose(false);
  filter->SetComputeInverseMeasure(true);
  filter->Update();

  if (filter->GetNumberOfLevels() != 3)
  {
    std::cerr << name << ": expected 3 levels, got " << filter->GetNumberOfLevels() << "\n";
    return false;
  }
  // The output has the type of the input, and its partitions the types of the input ones.
  const auto inputs = vtkCompositeDataSet::GetDataSets(input);
  const auto outputs = vtkCompositeDataSet::GetDataSets(filter->GetOutputDataObject(0));
  bool sameTypes =
    filter->GetOutputDataObject(0)->GetDataObjectType() == input->GetDataObjectType() &&
    outputs.size() == inputs.size();
  for (std::size_t i = 0; sameTypes && i < inputs.size(); ++i)
  {
    sameTypes = outputs[i]->GetDataObjectType() == inputs[i]->GetDataObjectType();
  }
  if (!sameTypes)
  {
    std::cerr << name << ": the output partitions differ from the input ones\n";
    return false;
  }
  auto output = Merge(filter->GetOutputDataObject(0));
  if (!CheckOutput<2>(name, output, false, 16, 8, THREE_LEVEL_DOMAIN))
  {
    return false;
  }
  const std::array<vtkIdType, 8> sizes = { 5, 4, 5, 5, 4, 4, 4, 4 };
  for (vtkIdType c = 0; c < 8; ++c)
  {
    if (output->GetCellSize(c) != sizes[c])
    {
      std::cerr << name << ": cell " << c << " should have " << sizes[c] << " points\n";
      return false;
    }
  }
  vtkDataArray* inverseArea = output->GetCellData()->GetArray("InverseArea");
  if (std::abs(inverseArea->GetTuple1(0) - 1.0 / 16.0) > EPSILON)
  {
    std::cerr << name << ": wrong InverseArea for the level 0 cell\n";
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
// Decomposition mode: cells 0, 2 and 3, which have 1 hanging point each, are each split into the
// fan of 3 triangles from it.
bool TestDecompositionMode(vtkDataObject* input, const std::string& name)
{
  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(input);
  filter->SetDecompose(true);
  filter->SetComputeInverseMeasure(true);
  filter->Update();

  if (filter->GetNumberOfLevels() != 3)
  {
    std::cerr << name << ": expected 3 levels, got " << filter->GetNumberOfLevels() << "\n";
    return false;
  }
  return CheckOutput<2>(
    name, Merge(filter->GetOutputDataObject(0)), true, 16, 14, THREE_LEVEL_DOMAIN);
}

// Conforming mesh: [0,4] x [0,4] and [4,8] x [0,4].
vtkSmartPointer<vtkPolyData> CreateConformingMesh()
{
  vtkNew<vtkPoints> points;
  points->SetDataType(VTK_DOUBLE);
  for (int i = 0; i < 3; ++i)
  {
    points->InsertNextPoint(Rotate(4.0 * i, 0).data());
    points->InsertNextPoint(Rotate(4.0 * i, 4).data());
  }
  vtkNew<vtkCellArray> polys;
  const vtkIdType q0[4] = { 0, 2, 3, 1 };
  const vtkIdType q1[4] = { 2, 4, 5, 3 };
  polys->InsertNextCell(4, q0);
  polys->InsertNextCell(4, q1);
  auto pd = vtkSmartPointer<vtkPolyData>::New();
  pd->SetPoints(points);
  pd->SetPolys(polys);
  return pd;
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
    if (filter->GetNumberOfLevels() != 1 || output->GetNumberOfPoints() != 6 ||
      output->GetNumberOfCells() != 2 || output->GetCellSize(0) != 4 || output->GetCellSize(1) != 4)
    {
      std::cerr << "ConformingInput: the mesh should be unchanged\n";
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
// An empty vtkPolyData or vtkUnstructuredGrid gives an empty output of its type.
bool TestEmptyInput()
{
  const std::vector<vtkSmartPointer<vtkPointSet>> inputs = { vtkSmartPointer<vtkPolyData>::New(),
    vtkSmartPointer<vtkUnstructuredGrid>::New() };
  for (vtkPointSet* input : inputs)
  {
    vtkNew<vtkResolveHangingNodes> filter;
    filter->SetInputData(input);
    filter->Update();
    vtkDataObject* output = filter->GetOutputDataObject(0);
    if (!output || output->GetDataObjectType() != input->GetDataObjectType() ||
      vtkDataSet::SafeDownCast(output)->GetNumberOfCells() != 0)
    {
      std::cerr << "EmptyInput: the output should be an empty " << input->GetClassName() << "\n";
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
// No inverse area is computed by default.
bool TestNoInverseMeasure()
{
  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(Split(CreateThreeLevelMesh(), THREE_LEVEL_SPLIT));
  filter->Update();
  for (vtkDataSet* output : vtkCompositeDataSet::GetDataSets(filter->GetOutputDataObject(0)))
  {
    if (output->GetCellData()->GetArray("InverseArea"))
    {
      std::cerr << "NoInverseMeasure: InverseArea should not be computed\n";
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
// PassThroughCellIds: off by default, and on, the cell array of OriginalCellIdsName holds the input
// cell of every output cell, i.e., the cell data "CellIds" it copies, also for the cells that a
// split cell is decomposed into.
bool TestPassThroughCellIds()
{
  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(CreateThreeLevelMesh());
  filter->Update();
  if (filter->GetPassThroughCellIds() ||
    vtkDataSet::SafeDownCast(filter->GetOutputDataObject(0))
      ->GetCellData()
      ->GetArray("vtkOriginalResolvedCellIds"))
  {
    std::cerr << "PassThroughCellIds: should be off by default\n";
    return false;
  }
  filter->SetPassThroughCellIds(true);
  filter->SetOriginalCellIdsName("InputCellIds");
  for (bool decompose : { false, true })
  {
    filter->SetDecompose(decompose);
    filter->Update();
    vtkCellData* cellData = vtkDataSet::SafeDownCast(filter->GetOutputDataObject(0))->GetCellData();
    vtkDataArray* originalCellIds = cellData->GetArray("InputCellIds");
    vtkDataArray* cellIds = cellData->GetArray("CellIds");
    bool success =
      originalCellIds && cellIds && originalCellIds->GetNumberOfTuples() == (decompose ? 14 : 8);
    for (vtkIdType c = 0; success && c < cellIds->GetNumberOfTuples(); ++c)
    {
      success = originalCellIds->GetTuple1(c) == cellIds->GetTuple1(c);
    }
    if (!success)
    {
      std::cerr << "PassThroughCellIds: wrong input cell ids, with Decompose " << decompose << "\n";
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
// Points of other processes: a point on the true boundary of a cell is inserted with its point
// data, while points away from the boundary and duplicates are ignored.
bool TestRemotePoints()
{
  auto input = CreateThreeLevelMesh();
  vtkNew<vtkPoints> remotePoints;
  remotePoints->SetDataType(VTK_DOUBLE);
  vtkNew<vtkDoubleArray> remoteValues;
  remoteValues->SetName("Values");
  auto addRemote = [&](double x, double y)
  {
    remotePoints->InsertNextPoint(Rotate(x, y).data());
    remoteValues->InsertNextValue(x + 10.0 * y);
  };
  addRemote(2, 0);   // on the bottom edge of cell 0
  addRemote(10, 10); // far away
  addRemote(2, 0);   // duplicate
  addRemote(2, 2);   // inside cell 0
  vtkNew<vtkPolyData> remote;
  remote->SetPoints(remotePoints);
  remote->GetPointData()->AddArray(remoteValues);

  vtkNew<vtkRemotePointsTestFilter> filter;
  filter->Remote = remote;
  filter->SetInputData(Split(input, THREE_LEVEL_SPLIT));
  filter->SetDecompose(false);
  filter->Update();
  auto output = Merge(filter->GetOutputDataObject(0));

  const vtkIdType numPoints = input->GetNumberOfPoints();
  if (output->GetNumberOfPoints() != numPoints + 1 || output->GetCellSize(0) != 6)
  {
    std::cerr << "RemotePoints: expected 1 more point, inserted in cell 0\n";
    return false;
  }
  const vtkIdType id = FindPoint(output, Rotate(2, 0));
  vtkDataArray* values = output->GetPointData()->GetArray("Values");
  if (id < 0 || !values || values->GetTuple1(id) != 2.0)
  {
    std::cerr << "RemotePoints: the remote point or its data is wrong\n";
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
// The inverse areas weight the interpolation of cell data to points, so that the value of a point
// does not depend on how many finer cells use it. The partitions are merged first, so that a point
// gets the cell data of all the cells that use it.
bool TestCellDataToPointData()
{
  auto input = CreateThreeLevelMesh();
  vtkNew<vtkDoubleArray> temperature;
  temperature->SetName("Temperature");
  for (vtkIdType i = 0; i < 8; ++i)
  {
    temperature->InsertNextValue(100.0 * (i + 1));
  }
  input->GetCellData()->AddArray(temperature);

  vtkNew<vtkResolveHangingNodes> resolver;
  resolver->SetInputData(Split(input, THREE_LEVEL_SPLIT));
  resolver->SetDecompose(false);
  resolver->SetComputeInverseMeasure(true);
  resolver->Update();

  vtkNew<vtkCellDataToPointData> cellToPoint;
  cellToPoint->SetInputData(Merge(resolver->GetOutputDataObject(0)));
  cellToPoint->SetEnableWeightedAverage(true);
  cellToPoint->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "InverseArea");
  cellToPoint->Update();

  auto output = vtkDataSet::SafeDownCast(cellToPoint->GetOutput());
  vtkDataArray* pointTemperature = output->GetPointData()->GetArray("Temperature");
  if (!pointTemperature)
  {
    std::cerr << "CellDataToPointData: missing point data\n";
    return false;
  }
  // The hanging point (4,2) is used by cell 0 (area 16), cell 1 and cell 3 (area 4).
  const double expected = (100.0 / 16 + 200.0 / 4 + 400.0 / 4) / (1.0 / 16 + 2.0 / 4);
  const vtkIdType hangingPoint = FindPoint(output, Rotate(4, 2));
  if (hangingPoint < 0 || std::abs(pointTemperature->GetTuple1(hangingPoint) - expected) > EPSILON)
  {
    std::cerr << "CellDataToPointData: wrong value at the hanging point\n";
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
// Point global ids before the other point arrays: the partitions exchange their points with their
// point data, global ids included, so both partitions keep their global ids, and a remote point
// gets the global id of the point it copies.
bool TestGlobalIds()
{
  auto mesh = CreateThreeLevelMesh(/*withGlobalIds*/ true);
  vtkNew<vtkResolveHangingNodes> filter;
  filter->SetInputData(Split(mesh, THREE_LEVEL_SPLIT));
  filter->SetDecompose(false);
  filter->SetComputeInverseMeasure(true);
  filter->Update();

  const auto outputs = vtkCompositeDataSet::GetDataSets(filter->GetOutputDataObject(0));
  if (outputs.size() != 2 || !outputs[0]->GetPointData()->GetGlobalIds() ||
    !outputs[1]->GetPointData()->GetGlobalIds())
  {
    std::cerr << "GlobalIds: both partitions should keep their global ids\n";
    return false;
  }
  if (!CheckOutput<2>(
        "GlobalIds", Merge(filter->GetOutputDataObject(0)), false, 16, 8, THREE_LEVEL_DOMAIN))
  {
    return false;
  }
  // The hanging point (7,2) of the other partition is copied with its point data.
  const vtkIdType id = FindPoint(outputs[0], Rotate(7, 2));
  vtkDataArray* values = outputs[0]->GetPointData()->GetArray("Values");
  vtkDataArray* globalIds = outputs[0]->GetPointData()->GetGlobalIds();
  if (id < 0 || !values || values->GetTuple1(id) != 27.0 ||
    globalIds->GetTuple1(id) != FindPoint(mesh, Rotate(7, 2)))
  {
    std::cerr << "GlobalIds: the remote point or its data is wrong\n";
    return false;
  }
  return true;
}
} // namespace

int TestResolveHangingNodes2DUG(int, char*[])
{
  auto polyData = CreateThreeLevelMesh();
  auto unstructuredGrid = ToUnstructuredGrid(polyData);
  if (BoundaryMeasure<2>(polyData) < THREE_LEVEL_DOMAIN.BoundaryMeasure + 1.0)
  {
    std::cerr << "The input should have hanging nodes\n";
    return EXIT_FAILURE;
  }
  const std::map<std::string, vtkSmartPointer<vtkDataObject>> inputs = {
    { "vtkPolyData", polyData },
    { "vtkUnstructuredGrid", unstructuredGrid },
    { "vtkPolyData partitions", Split(polyData, THREE_LEVEL_SPLIT) },
    { "vtkUnstructuredGrid partitions", Split(unstructuredGrid, THREE_LEVEL_SPLIT) },
  };
  for (const auto& [inputName, input] : inputs)
  {
    if (!TestPolygonMode(input, "PolygonMode(" + inputName + ")") ||
      !TestDecompositionMode(input, "DecompositionMode(" + inputName + ")"))
    {
      return EXIT_FAILURE;
    }
  }
  if (!TestConformingInput() || !TestEmptyInput() || !TestNoInverseMeasure() ||
    !TestPassThroughCellIds() || !TestRemotePoints() || !TestCellDataToPointData() ||
    !TestGlobalIds())
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
