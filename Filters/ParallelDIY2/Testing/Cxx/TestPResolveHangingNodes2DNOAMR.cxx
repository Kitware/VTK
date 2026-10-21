// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkPResolveHangingNodes on a 2D vtkNonOverlappingAMR of vtkImageData blocks with 3 levels
// distributed over 4 processes, with a block each: polygon and decomposition modes, a block only
// receiving the points of the blocks of the other levels, of other processes, with their point
// data, and the conformity of the gathered output.

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "PResolveHangingNodesTestUtilities.h"

#include "vtkDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <iostream>
#include <string>
#include <vector>

namespace
{
// The domain [0,8] x [0,4].
constexpr double DOMAIN_AREA = 32.0;
constexpr double DOMAIN_PERIMETER = 24.0;

// The numbers of blocks of the levels of the AMR.
const std::vector<unsigned int> BLOCKS_PER_LEVEL = { 1, 2, 1 };

// The block of every process: its level, its index in the level, and its expected output: the
// number of points, and the hanging point received from another level, if any.
struct Block
{
  unsigned int Level;
  unsigned int Index;
  std::array<double, 3> Origin;
  std::array<int, 3> NumberOfCells;
  double Spacing;
  vtkIdType NumberOfPoints;
  bool HasRemotePoint;
  std::array<double, 3> RemotePoint;
};

// 3-level AMR over the domain [0,8] x [0,4], with a block on each process:
// - Process 0, level 0: [0,4] x [0,4], 2 x 2 cells of size 2. Its edge x = 4 has the hanging
//   points (4,1) and (4,3) of process 1.
// - Processes 1 and 2, level 1: A = [4,6] x [0,4], 2 x 4 cells of size 1, whose edge x = 6 has
//   the hanging points (6,2.5) and (6,3.5) of process 3, and B = [6,8] x [0,2], 2 x 2 cells of
//   size 1, whose edge y = 2 has the hanging points (6.5,2) and (7.5,2) of process 3. A and B are
//   conforming, and do not exchange their points.
// - Process 3, level 2: [6,8] x [2,4], 4 x 4 cells of size 0.5.
const std::array<Block, 4> BLOCKS = { {
  { 0, 0, { 0, 0, 0 }, { 2, 2, 0 }, 2.0, 9 + 2, true, { 4, 1, 0 } },
  { 1, 0, { 4, 0, 0 }, { 2, 4, 0 }, 1.0, 15 + 2, true, { 6, 2.5, 0 } },
  { 1, 1, { 6, 0, 0 }, { 2, 2, 0 }, 1.0, 9 + 2, true, { 6.5, 2, 0 } },
  { 2, 0, { 6, 2, 0 }, { 4, 4, 0 }, 0.5, 25, false, { 0, 0, 0 } },
} };

// The AMR of the process `rank`, with all the levels but only its block.
vtkSmartPointer<vtkNonOverlappingAMR> CreateAMR(int rank)
{
  const Block& block = BLOCKS[rank];
  auto amr = vtkSmartPointer<vtkNonOverlappingAMR>::New();
  amr->Initialize(BLOCKS_PER_LEVEL);
  amr->SetDataSet(block.Level, block.Index,
    CreateBlock(static_cast<int>(block.Level), block.Origin, block.NumberOfCells, block.Spacing));
  return amr;
}

//------------------------------------------------------------------------------
// Polygon and decomposition modes: the blocks of the levels 0 and 1 get the hanging points of the
// finer blocks of other processes, but not the points of the blocks of their own level.
bool TestMode(vtkMultiProcessController* controller, bool decompose)
{
  const std::string name = decompose ? "DecompositionMode" : "PolygonMode";
  const int rank = controller->GetLocalProcessId();
  const Block& block = BLOCKS[rank];
  vtkNew<vtkPLevelsTestFilter> filter;
  filter->SetController(controller);
  filter->SetInputData(CreateAMR(rank));
  filter->SetDecompose(decompose);
  filter->SetComputeInverseMeasure(true);
  filter->Update();
  auto output = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));

  bool success = true;
  if (filter->GetNumberOfLevels() != 3)
  {
    std::cerr << name << ": process " << rank << " expected 3 levels, got "
              << filter->GetNumberOfLevels() << "\n";
    success = false;
  }
  if (filter->PartitionedDataSetIds != std::vector<int>{ static_cast<int>(block.Level) } ||
    !filter->ReceivedOnlyOtherLevels)
  {
    std::cerr << name << ": process " << rank
              << " should only receive the points of the other levels\n";
    success = false;
  }
  auto blockOutput = vtkUnstructuredGrid::SafeDownCast(
    output ? output->GetPartition(block.Level, block.Index) : nullptr);
  if (!blockOutput || blockOutput->GetNumberOfPoints() != block.NumberOfPoints)
  {
    std::cerr << name << ": process " << rank << " should have a vtkUnstructuredGrid with "
              << block.NumberOfPoints << " points\n";
    success = false;
  }
  else if (block.HasRemotePoint)
  {
    // The hanging point of the finer level is copied with its point data.
    const vtkIdType id = FindPoint(blockOutput, block.RemotePoint);
    vtkDataArray* levels = blockOutput->GetPointData()->GetArray("Levels");
    if (id < 0 || !levels || levels->GetTuple1(id) != block.Level + 1)
    {
      std::cerr << name << ": process " << rank << " received a wrong point or point data\n";
      success = false;
    }
  }
  // All the processes take part in the gather, even if a local check failed.
  success =
    CheckGlobalOutput<2>(name, controller, output, DOMAIN_AREA, DOMAIN_PERIMETER) && success;
  return success;
}
} // namespace

int TestPResolveHangingNodes2DNOAMR(int argc, char* argv[])
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
    // All the processes report the same result.
    int globalSuccess = 0;
    controller->AllReduce(&success, &globalSuccess, 1, vtkCommunicator::MIN_OP);
    success = globalSuccess;
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  controller->Finalize();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
