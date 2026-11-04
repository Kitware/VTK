// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests the point global ids of vtkPResolveHangingNodes on a 3D vtkUnstructuredGrid mesh of
// hexahedra with 2 levels distributed over 4 processes, whose coarse cells are decomposed with a
// point at their center: the partitions keep their global ids, the remote points get the global
// ids of the points they copy, and the center points get new global ids, after the largest one of
// the whole input, unique over all the processes and over all the vtkPartitionedDataSet of a
// vtkPartitionedDataSetCollection.

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "PResolveHangingNodesTestUtilities.h"

#include "vtkCellType.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPResolveHangingNodes.h"
#include "vtkPartitionedDataSet.h"
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

namespace
{
// The domain of the stacked L-shaped mesh: [0,4] x [0,2] x [0,4] and [0,2] x [2,4] x [0,4].
constexpr double DOMAIN_VOLUME = 48.0;
constexpr double DOMAIN_SURFACE = 88.0;

// The global id of the point (x, y, z), the same on all the processes.
vtkIdType GlobalId(int x, int y, int z)
{
  return x + 5 * y + 25 * z;
}

// The partition of the process `rank` of the L-shaped mesh stacked twice along z, from `z0`: the
// coarse cells A ([0,2] x [0,2] x [z0,z0+2], rank 0) and A' (z in [z0+2,z0+4], rank 1) border the
// finer cells D ([2,4] x [0,2] x [z0,z0+4], rank 2) and E ([0,2] x [2,4] x [z0,z0+4], rank 3)
// across their faces x = 2 and y = 2, so both are decomposed with a point at their center, on
// different processes than the finer cells. The point global ids come before the point data
// "Values", x + 10 y + 100 z.
vtkSmartPointer<vtkUnstructuredGrid> CreatePartition(int rank, int z0)
{
  vtkNew<vtkPoints> points;
  points->SetDataType(VTK_DOUBLE);
  vtkNew<vtkIdTypeArray> globalIds;
  globalIds->SetName("GlobalIds");
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
    globalIds->InsertNextValue(GlobalId(x, y, z));
    values->InsertNextValue(x + 10.0 * y + 100.0 * z);
    const vtkIdType id = points->InsertNextPoint(x, y, z);
    pointIds[{ x, y, z }] = id;
    return id;
  };

  auto ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
  ug->AllocateExact(16, 8);
  auto hex = [&](int x, int y, int z, int size)
  {
    const vtkIdType ids[8] = { point(x, y, z), point(x + size, y, z), point(x + size, y + size, z),
      point(x, y + size, z), point(x, y, z + size), point(x + size, y, z + size),
      point(x + size, y + size, z + size), point(x, y + size, z + size) };
    ug->InsertNextCell(VTK_HEXAHEDRON, 8, ids);
  };

  if (rank < 2)
  {
    hex(0, 0, z0 + 2 * rank, 2);
  }
  else
  {
    const int x0 = rank == 2 ? 2 : 0;
    const int y0 = rank == 2 ? 0 : 2;
    for (int k = 0; k < 4; ++k)
    {
      for (int j = 0; j < 2; ++j)
      {
        for (int i = 0; i < 2; ++i)
        {
          hex(x0 + i, y0 + j, z0 + k, 1);
        }
      }
    }
  }
  ug->SetPoints(points);
  ug->GetPointData()->SetGlobalIds(globalIds);
  ug->GetPointData()->AddArray(values);
  return ug;
}

// Checks the global ids of `output`, the output of the partition of the process `rank`: the points
// of the input, local or remote, have the global ids of their positions, and the process of a
// coarse cell has 1 center point, of global id `centerGlobalId`.
bool CheckGlobalIds(
  const std::string& name, int rank, vtkUnstructuredGrid* output, vtkIdType centerGlobalId)
{
  vtkDataArray* globalIds = output ? output->GetPointData()->GetGlobalIds() : nullptr;
  if (!globalIds)
  {
    std::cerr << name << ": process " << rank << " has no point global ids\n";
    return false;
  }
  vtkIdType numCenterPoints = 0;
  for (vtkIdType i = 0; i < output->GetNumberOfPoints(); ++i)
  {
    const double* p = output->GetPoint(i);
    const vtkIdType globalId = static_cast<vtkIdType>(globalIds->GetTuple1(i));
    // The points of the input are on the integer lattice, the center points are not: the center
    // of A is the average of its 8 corners and 9 hanging points, (21/17, 21/17, z0 + 1), and the
    // center of A' is above it.
    const std::array<double, 3> lattice = { std::round(p[0]), std::round(p[1]), std::round(p[2]) };
    if (vtkMath::Distance2BetweenPoints(p, lattice.data()) < EPSILON)
    {
      const vtkIdType expected = GlobalId(
        static_cast<int>(lattice[0]), static_cast<int>(lattice[1]), static_cast<int>(lattice[2]));
      if (globalId != expected)
      {
        std::cerr << name << ": process " << rank << " has the point (" << p[0] << ", " << p[1]
                  << ", " << p[2] << ") of global id " << globalId << " instead of " << expected
                  << "\n";
        return false;
      }
      continue;
    }
    if (globalId != centerGlobalId)
    {
      std::cerr << name << ": process " << rank << " has the point (" << p[0] << ", " << p[1]
                << ", " << p[2] << ") of global id " << globalId << ", expected " << centerGlobalId
                << " for a center point\n";
      return false;
    }
    ++numCenterPoints;
  }
  const vtkIdType expectedCenterPoints = rank < 2 ? 1 : 0;
  if (numCenterPoints != expectedCenterPoints)
  {
    std::cerr << name << ": process " << rank << " has " << numCenterPoints
              << " center points instead of " << expectedCenterPoints << "\n";
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
// A vtkPartitionedDataSet: the largest global id of the input is the one of (2,4,4), 122, so the
// centers of A (rank 0) and A' (rank 1) get 123 and 124, in the order of the processes.
bool TestPartitionedDataSet(vtkMultiProcessController* controller)
{
  const std::string name = "PartitionedDataSet";
  const int rank = controller->GetLocalProcessId();
  vtkNew<vtkPartitionedDataSet> input;
  input->SetPartition(0, CreatePartition(rank, 0));
  vtkNew<vtkPResolveHangingNodes> filter;
  filter->SetController(controller);
  filter->SetInputData(input);
  filter->SetComputeInverseMeasure(true);
  filter->Update();
  vtkSmartPointer<vtkPartitionedDataSet> output =
    vtkPartitionedDataSet::SafeDownCast(filter->GetOutputDataObject(0));

  bool success = CheckGlobalIds(name, rank,
    output ? vtkUnstructuredGrid::SafeDownCast(output->GetPartition(0)) : nullptr,
    GlobalId(2, 4, 4) + 1 + rank);
  if (!output)
  {
    // All the processes take part in the gather.
    output = vtkSmartPointer<vtkPartitionedDataSet>::New();
  }
  success =
    CheckGlobalOutput<3>(name, controller, output, DOMAIN_VOLUME, DOMAIN_SURFACE) && success;
  return success;
}

//------------------------------------------------------------------------------
// A vtkPartitionedDataSetCollection of 2 vtkPartitionedDataSet, resolved on their own: the stacked
// L-shaped mesh, and the same mesh above it, z in [4,8]. The largest global id of the whole input
// is the one of (2,4,8), 222, so the centers of the first vtkPartitionedDataSet get 223 and 224,
// and those of the second one 225 and 226, although its own largest global id is 222 too and the
// first one only has global ids up to 122.
bool TestCollection(vtkMultiProcessController* controller)
{
  const std::string name = "Collection";
  const int rank = controller->GetLocalProcessId();
  vtkNew<vtkPartitionedDataSetCollection> input;
  for (unsigned int i = 0; i < 2; ++i)
  {
    input->SetPartition(i, 0, CreatePartition(rank, 4 * static_cast<int>(i)));
  }
  vtkNew<vtkPResolveHangingNodes> filter;
  filter->SetController(controller);
  filter->SetInputData(input);
  filter->SetComputeInverseMeasure(true);
  filter->Update();
  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));

  bool success = output && output->GetNumberOfPartitionedDataSets() == 2;
  if (!success)
  {
    std::cerr << name << ": process " << rank
              << " should have an output of the structure of its input\n";
    // All the processes take part in the gathers.
    output = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
    output->SetNumberOfPartitionedDataSets(2);
  }
  for (unsigned int i = 0; i < 2; ++i)
  {
    const std::string pdsName = name + "(" + vtk::to_string(i) + ")";
    auto partition = vtkUnstructuredGrid::SafeDownCast(output->GetPartition(i, 0));
    const vtkIdType centerGlobalId = GlobalId(2, 4, 8) + 1 + 2 * static_cast<vtkIdType>(i) + rank;
    success = CheckGlobalIds(pdsName, rank, partition, centerGlobalId) && success;
    success = CheckGlobalOutput<3>(pdsName, controller, output->GetPartitionedDataSet(i),
                DOMAIN_VOLUME, DOMAIN_SURFACE) &&
      success;
  }
  return success;
}
} // namespace

int TestPResolveHangingNodes3DGlobalIds(int argc, char* argv[])
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
    success = TestPartitionedDataSet(controller) ? 1 : 0;
    success = TestCollection(controller) && success;
    // All the processes report the same result.
    int globalSuccess = 0;
    controller->AllReduce(&success, &globalSuccess, 1, vtkCommunicator::MIN_OP);
    success = globalSuccess;
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  controller->Finalize();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
