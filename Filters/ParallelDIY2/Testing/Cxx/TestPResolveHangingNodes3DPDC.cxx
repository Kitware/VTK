// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkPResolveHangingNodes on a vtkPartitionedDataSetCollection of a 3D vtkUnstructuredGrid
// mesh of hexahedra with 2 levels distributed over 4 processes: the output has the structure of the
// input, with null partitions and a vtkPartitionedDataSet without partitions on every process, the
// partitions of a vtkPartitionedDataSet are resolved together across the processes, and every
// vtkPartitionedDataSet on its own.

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "PResolveHangingNodesTestUtilities.h"

#include "vtkCellType.h"
#include "vtkDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPResolveHangingNodes.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <iostream>
#include <string>

namespace
{
// The domain [0,4] x [0,4] x [0,2].
constexpr double DOMAIN_VOLUME = 32.0;
constexpr double DOMAIN_SURFACE = 64.0;

// The coarse partitions 0, 2 and 3 (A, B and C): [0,4] x [0,4] x [0,2] without [2,4] x [0,2] x
// [0,2].
constexpr double COARSE_VOLUME = 24.0;
constexpr double COARSE_SURFACE = 56.0;

// The input of the process `rank`, a vtkPartitionedDataSetCollection of 3 vtkPartitionedDataSet:
// - 0: no partitions, on every process.
// - 1: the partitions 2 rank, a null partition, and 2 rank + 1 of the 2-level mesh.
// - 2: the coarse partitions of the 2-level mesh, 0 on process 0, and 2 and 3 on process 1, and no
//   partitions on the others.
vtkSmartPointer<vtkPartitionedDataSetCollection> CreateInput(int rank)
{
  vtkNew<vtkPartitionedDataSet> mesh;
  mesh->SetPartition(0, CreateHexPartition(2 * rank));
  mesh->SetPartition(1, nullptr);
  mesh->SetPartition(2, CreateHexPartition(2 * rank + 1));
  vtkNew<vtkPartitionedDataSet> coarse;
  if (rank == 0)
  {
    coarse->SetPartition(0, CreateHexPartition(0));
  }
  else if (rank == 1)
  {
    coarse->SetPartition(0, CreateHexPartition(2));
    coarse->SetPartition(1, CreateHexPartition(3));
  }
  auto input = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
  input->SetPartitionedDataSet(0, vtkSmartPointer<vtkPartitionedDataSet>::New());
  input->SetPartitionedDataSet(1, mesh);
  input->SetPartitionedDataSet(2, coarse);
  return input;
}

//------------------------------------------------------------------------------
// The output has the structure of the input. The whole mesh is conforming once gathered, while the
// coarse partitions alone are unchanged, since their hanging points are in the partitions of D of
// the other vtkPartitionedDataSet, here (partition 1 for A) or on other processes.
bool TestSeparatePartitionedDataSets(vtkMultiProcessController* controller, bool decompose)
{
  const std::string name =
    std::string("SeparatePartitionedDataSets") + (decompose ? "(decomposition)" : "(polyhedron)");
  const int rank = controller->GetLocalProcessId();
  vtkSmartPointer<vtkPartitionedDataSetCollection> input = CreateInput(rank);
  vtkNew<vtkPResolveHangingNodes> filter;
  filter->SetController(controller);
  filter->SetInputData(input);
  filter->SetDecompose(decompose);
  filter->SetComputeInverseMeasure(true);
  filter->Update();
  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));

  bool success = output && output->GetNumberOfPartitionedDataSets() == 3;
  for (unsigned int i = 0; success && i < 3; ++i)
  {
    success = output->GetNumberOfPartitions(i) == input->GetNumberOfPartitions(i);
  }
  success = success && vtkUnstructuredGrid::SafeDownCast(output->GetPartition(1, 0)) &&
    !output->GetPartition(1, 1) && vtkUnstructuredGrid::SafeDownCast(output->GetPartition(1, 2));
  if (!success)
  {
    std::cerr << name << ": process " << rank
              << " should have an output of the structure of its input\n";
    // All the processes take part in the gathers.
    output = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
    output->SetNumberOfPartitionedDataSets(3);
  }
  for (unsigned int i = 0; success && i < output->GetNumberOfPartitions(2); ++i)
  {
    vtkDataSet* partition = output->GetPartition(2, i);
    if (partition->GetNumberOfPoints() != 8 || partition->GetNumberOfCells() != 1 ||
      partition->GetCellType(0) != VTK_HEXAHEDRON)
    {
      std::cerr << name << ": process " << rank << " has a coarse partition that is changed\n";
      success = false;
    }
  }
  success = CheckGlobalOutput<3>(name + "(mesh)", controller, output->GetPartitionedDataSet(1),
              DOMAIN_VOLUME, DOMAIN_SURFACE) &&
    success;
  success = CheckGlobalOutput<3>(name + "(coarse)", controller, output->GetPartitionedDataSet(2),
              COARSE_VOLUME, COARSE_SURFACE) &&
    success;
  return success;
}
} // namespace

int TestPResolveHangingNodes3DPDC(int argc, char* argv[])
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
    success = TestSeparatePartitionedDataSets(controller, false) ? 1 : 0;
    success = TestSeparatePartitionedDataSets(controller, true) && success;
    // All the processes report the same result.
    int globalSuccess = 0;
    controller->AllReduce(&success, &globalSuccess, 1, vtkCommunicator::MIN_OP);
    success = globalSuccess;
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  controller->Finalize();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
