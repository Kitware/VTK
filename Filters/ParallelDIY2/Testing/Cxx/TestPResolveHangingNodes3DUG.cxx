// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkPResolveHangingNodes on a 3D vtkUnstructuredGrid mesh of hexahedra with 2 levels
// distributed over 4 processes, with 2 partitions each of a vtkPartitionedDataSetCollection:
// polyhedron and decomposition modes, points received from the other partitions of the same
// process and of other processes with their point data, the conformity of the gathered output, and
// a process without cells.

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "PResolveHangingNodesTestUtilities.h"

#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPResolveHangingNodes.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace
{
// The domain [0,4] x [0,4] x [0,2].
constexpr double DOMAIN_VOLUME = 32.0;
constexpr double DOMAIN_SURFACE = 64.0;

// 2-level mesh over the domain [0,4] x [0,4] x [0,2], in 8 partitions, the partitions 2 rank and
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
vtkSmartPointer<vtkUnstructuredGrid> CreatePartition(int partition)
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

// The partitions of the process `rank`, in the single vtkPartitionedDataSet of a
// vtkPartitionedDataSetCollection.
vtkSmartPointer<vtkPartitionedDataSetCollection> CreatePartitions(int rank)
{
  auto partitions = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
  for (unsigned int i = 0; i < 2; ++i)
  {
    partitions->SetPartition(0, i, CreatePartition(2 * rank + static_cast<int>(i)));
  }
  return partitions;
}

// Expected output of a partition: the numbers of points and cells, and one point received from
// another partition, if any.
struct Expected
{
  vtkIdType NumberOfPoints;
  vtkIdType NumberOfPolyhedronModeCells;
  vtkIdType NumberOfDecompositionModeCells;
  bool HasRemotePoint;
  std::array<double, 3> RemotePoint;
  double RemoteValue;
};

// A and C get the 4 points on the edges and the point at the center of their face bordering D
// (5 pyramids and 4 tetrahedra), B gets the point on its edge (4 pyramids), D is unchanged.
const std::array<Expected, 8> EXPECTED = { {
  { 13, 1, 9, true, { 2, 1, 1 }, 2 + 10 + 100 + 1000 }, // face center from partition 1
  { 12, 2, 2, false, { 0, 0, 0 }, 0 },                  // unchanged
  { 9, 1, 4, true, { 2, 2, 1 }, 2 + 20 + 100 + 4000 },  // edge point from partition 4
  { 13, 1, 9, true, { 3, 2, 1 }, 3 + 20 + 100 + 4000 }, // face center from partition 4
  { 12, 2, 2, false, { 0, 0, 0 }, 0 },                  // unchanged
  { 12, 2, 2, false, { 0, 0, 0 }, 0 },                  // unchanged
  { 8, 1, 1, false, { 0, 0, 0 }, 0 },                   // unchanged
  { 8, 1, 1, false, { 0, 0, 0 }, 0 },                   // unchanged
} };

// Checks of the output of a partition of this process.
bool CheckLocalOutput(const std::string& name, int partition, vtkResolveHangingNodes* filter,
  vtkUnstructuredGrid* output, bool decompose)
{
  const Expected& expected = EXPECTED[partition];
  const vtkIdType expectedCells =
    decompose ? expected.NumberOfDecompositionModeCells : expected.NumberOfPolyhedronModeCells;
  if (filter->GetNumberOfLevels() != 2)
  {
    std::cerr << name << ": partition " << partition << " expected 2 levels, got "
              << filter->GetNumberOfLevels() << "\n";
    return false;
  }
  if (!output)
  {
    std::cerr << name << ": partition " << partition << " has no vtkUnstructuredGrid output\n";
    return false;
  }
  if (output->GetNumberOfPoints() != expected.NumberOfPoints ||
    output->GetNumberOfCells() != expectedCells)
  {
    std::cerr << name << ": partition " << partition << " expected " << expected.NumberOfPoints
              << " points and " << expectedCells << " cells, got " << output->GetNumberOfPoints()
              << " and " << output->GetNumberOfCells() << "\n";
    return false;
  }
  for (vtkIdType c = 0; c < output->GetNumberOfCells(); ++c)
  {
    const int type = output->GetCellType(c);
    if (type != VTK_HEXAHEDRON &&
      (decompose ? (type != VTK_TETRA && type != VTK_PYRAMID) : type != VTK_POLYHEDRON))
    {
      std::cerr << name << ": partition " << partition << " has a cell of type " << type << "\n";
      return false;
    }
  }
  if (expected.HasRemotePoint)
  {
    // The point received from another partition is copied with its point data.
    const vtkIdType id = FindPoint(output, expected.RemotePoint);
    vtkDataArray* values = output->GetPointData()->GetArray("Values");
    if (id < 0 || !values || std::abs(values->GetTuple1(id) - expected.RemoteValue) > EPSILON)
    {
      std::cerr << name << ": partition " << partition << " received a wrong point or point data\n";
      return false;
    }
  }
  if (!output->GetCellData()->GetArray("InverseVolume"))
  {
    std::cerr << name << ": partition " << partition << " has no InverseVolume cell data array\n";
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
// Polyhedron and decomposition modes, every partition having cells.
bool TestMode(vtkMultiProcessController* controller, bool decompose)
{
  const std::string name = decompose ? "DecompositionMode" : "PolyhedronMode";
  const int rank = controller->GetLocalProcessId();
  vtkNew<vtkPResolveHangingNodes> filter;
  filter->SetController(controller);
  filter->SetInputData(CreatePartitions(rank));
  filter->SetDecompose(decompose);
  filter->SetComputeInverseMeasure(true);
  filter->Update();
  auto output = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));

  bool success = true;
  for (unsigned int i = 0; i < 2; ++i)
  {
    const int partition = 2 * rank + static_cast<int>(i);
    auto partitionOutput = vtkUnstructuredGrid::SafeDownCast(output->GetPartition(0, i));
    success = CheckLocalOutput(name, partition, filter, partitionOutput, decompose) && success;
  }
  // All the processes take part in the gather, even if a local check failed.
  success =
    CheckGlobalOutput<3>(name, controller, output, DOMAIN_VOLUME, DOMAIN_SURFACE) && success;
  return success;
}

//------------------------------------------------------------------------------
// A process without cells must take part in the exchange of points and in the computation of the
// number of levels: process 1 has `numEmptyPartitions` partitions without cells instead of B and C
// (with none, it has a diy block without partition, see
// vtkPResolveHangingNodes::GetRemotePoints), so the domain is [0,4] x [0,2] x [0,2], of volume 16
// and surface 40.
bool TestEmptyProcess(vtkMultiProcessController* controller, unsigned int numEmptyPartitions)
{
  const std::string name = "EmptyProcess(" + vtk::to_string(numEmptyPartitions) + " partitions)";
  const int rank = controller->GetLocalProcessId();
  vtkSmartPointer<vtkPartitionedDataSetCollection> input = CreatePartitions(rank);
  if (rank == 1)
  {
    input->SetNumberOfPartitions(0, numEmptyPartitions);
    for (unsigned int i = 0; i < numEmptyPartitions; ++i)
    {
      input->SetPartition(0, i, vtkSmartPointer<vtkUnstructuredGrid>::New());
    }
  }
  vtkNew<vtkPResolveHangingNodes> filter;
  filter->SetController(controller);
  filter->SetInputData(input);
  filter->SetComputeInverseMeasure(true);
  filter->Update();
  auto output = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));

  bool success = output->GetNumberOfPartitions(0) == input->GetNumberOfPartitions(0);
  if (!success)
  {
    std::cerr << name << ": process " << rank << " has " << output->GetNumberOfPartitions(0)
              << " output partitions instead of " << input->GetNumberOfPartitions(0) << "\n";
  }
  for (unsigned int i = 0; success && i < output->GetNumberOfPartitions(0); ++i)
  {
    const int partition = 2 * rank + static_cast<int>(i);
    vtkDataSet* partitionOutput = output->GetPartition(0, i);
    if (!partitionOutput ||
      (rank == 1 ? partitionOutput->GetNumberOfCells() != 0
                 : partitionOutput->GetNumberOfPoints() != EXPECTED[partition].NumberOfPoints))
    {
      std::cerr << name << ": partition " << partition << " has a wrong output\n";
      success = false;
    }
  }
  // The process without cells uses the dimension of the others for the number of levels.
  if (filter->GetNumberOfLevels() != 2)
  {
    std::cerr << name << ": process " << rank << " has " << filter->GetNumberOfLevels()
              << " levels instead of 2\n";
    success = false;
  }
  success =
    CheckGlobalOutput<3>(name, controller, output, DOMAIN_VOLUME - 16.0, DOMAIN_SURFACE - 24.0) &&
    success;
  return success;
}
} // namespace

int TestPResolveHangingNodes3DUG(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> controller;
#else
  vtkNew<vtkDummyController> controller;
#endif
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  int success = 1;
  if (controller->GetNumberOfProcesses() != 4)
  {
    std::cerr << "This test needs 4 processes, running with " << controller->GetNumberOfProcesses()
              << "\n";
    success = 0;
  }
  else
  {
    success = TestMode(controller, false) ? 1 : 0;
    success = TestMode(controller, true) && success;
    success = TestEmptyProcess(controller, 2) && success;
    success = TestEmptyProcess(controller, 0) && success;
    // All the processes report the same result.
    int globalSuccess = 0;
    controller->AllReduce(&success, &globalSuccess, 1, vtkCommunicator::MIN_OP);
    success = globalSuccess;
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  controller->Finalize();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
