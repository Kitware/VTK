// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAMRBox.h"
#include "vtkAMRGaussianPulseSource.h"
#include "vtkAMRInterfaceFilter.h"
#include "vtkAMRUtilities.h"
#include "vtkCellData.h"
#include "vtkCellDataToPointData.h"
#include "vtkCommunicator.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredData.h"
#include "vtkTestUtilities.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
//------------------------------------------------------------------------------
vtkSmartPointer<vtkOverlappingAMR> MakePulse()
{
  vtkNew<vtkAMRGaussianPulseSource> source;
  vtkNew<vtkCellDataToPointData> cellToPoint;
  cellToPoint->SetInputConnection(source->GetOutputPort());
  cellToPoint->Update();
  return vtkOverlappingAMR::SafeDownCast(cellToPoint->GetOutputDataObject(0));
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkUniformGrid> MakeBox(const vtkAMRBox& box, double h)
{
  auto grid = vtkSmartPointer<vtkUniformGrid>::New();
  const int* lo = box.GetLoCorner();
  int dimensions[3];
  box.GetNumberOfNodes(dimensions);
  grid->SetOrigin(lo[0] * h, lo[1] * h, lo[2] * h);
  grid->SetSpacing(h, h, h);
  grid->SetDimensions(dimensions);

  vtkNew<vtkDoubleArray> values;
  values->SetName("f");
  values->SetNumberOfTuples(grid->GetNumberOfPoints());
  for (vtkIdType pointId = 0; pointId < grid->GetNumberOfPoints(); ++pointId)
  {
    double x[3];
    grid->GetPoint(pointId, x);
    values->SetValue(pointId, std::sin(x[0]) + std::cos(x[1]) + 0.1 * x[2]);
  }
  grid->GetPointData()->SetScalars(values);
  return grid;
}

//------------------------------------------------------------------------------
// An AMR with many blocks per level: 4 x 4 x 2 coarse boxes, the middle of which is refined by 8
// boxes, so blocks of every process border the refined region.
vtkSmartPointer<vtkOverlappingAMR> MakeManyBlocks()
{
  constexpr int coarseCells = 4;
  constexpr double h0 = 1.0;
  constexpr int ratio = 2;

  auto amr = vtkSmartPointer<vtkOverlappingAMR>::New();
  amr->Initialize({ 32, 8 });
  const double origin[3] = { 0.0, 0.0, 0.0 };
  amr->SetOrigin(origin);
  amr->SetGridDescription(vtkStructuredData::VTK_STRUCTURED_XYZ_GRID);
  const double spacing0[3] = { h0, h0, h0 };
  const double spacing1[3] = { h0 / ratio, h0 / ratio, h0 / ratio };
  amr->SetSpacing(0, spacing0);
  amr->SetSpacing(1, spacing1);
  amr->SetRefinementRatio(0, ratio);

  unsigned int idx = 0;
  for (int k = 0; k < 2; ++k)
  {
    for (int j = 0; j < 4; ++j)
    {
      for (int i = 0; i < 4; ++i, ++idx)
      {
        const int lo[3] = { i * coarseCells, j * coarseCells, k * coarseCells };
        const int hi[3] = { lo[0] + coarseCells - 1, lo[1] + coarseCells - 1,
          lo[2] + coarseCells - 1 };
        const vtkAMRBox box(lo, hi);
        amr->SetAMRBox(0, idx, box);
        amr->SetDataSet(0, idx, ::MakeBox(box, h0));
      }
    }
  }

  // The coarse cells [4, 11] x [4, 11] x [0, 7] are refined.
  constexpr int fineCells = 8;
  idx = 0;
  for (int k = 0; k < 2; ++k)
  {
    for (int j = 0; j < 2; ++j)
    {
      for (int i = 0; i < 2; ++i, ++idx)
      {
        const int lo[3] = { 8 + i * fineCells, 8 + j * fineCells, k * fineCells };
        const int hi[3] = { lo[0] + fineCells - 1, lo[1] + fineCells - 1, lo[2] + fineCells - 1 };
        const vtkAMRBox box(lo, hi);
        amr->SetAMRBox(1, idx, box);
        amr->SetDataSet(1, idx, ::MakeBox(box, h0 / ratio));
      }
    }
  }

  vtkAMRUtilities::BlankCells(amr);
  return amr;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPartitionedDataSet> ComputeInterface(
  vtkOverlappingAMR* amr, vtkMultiProcessController* controller)
{
  vtkNew<vtkAMRInterfaceFilter> interface;
  interface->SetController(controller);
  interface->SetInputData(amr);
  interface->Update();
  return interface->GetOutput();
}

//------------------------------------------------------------------------------
// The interface pieces of the output of vtkAMRInterfaceFilter. The output holds the grids of the
// finest level and the interfaces of the others, with empty partitions removed.
std::vector<vtkUnstructuredGrid*> GetInterfaces(vtkPartitionedDataSet* output)
{
  std::vector<vtkUnstructuredGrid*> interfaces;
  for (unsigned int partitionIdx = 0; partitionIdx < output->GetNumberOfPartitions();
       ++partitionIdx)
  {
    if (auto interface = vtkUnstructuredGrid::SafeDownCast(output->GetPartition(partitionIdx)))
    {
      interfaces.push_back(interface);
    }
  }
  return interfaces;
}

//------------------------------------------------------------------------------
// The grid with one layer of duplicate ghost cells around it, holding copies of its border values.
vtkSmartPointer<vtkUniformGrid> AddGhostLayer(vtkUniformGrid* grid)
{
  int extent[6], padded[6];
  grid->GetExtent(extent);
  for (int d = 0; d < 3; ++d)
  {
    padded[2 * d] = extent[2 * d] - 1;
    padded[2 * d + 1] = extent[2 * d + 1] + 1;
  }

  auto output = vtkSmartPointer<vtkUniformGrid>::New();
  output->SetOrigin(grid->GetOrigin());
  output->SetSpacing(grid->GetSpacing());
  output->SetExtent(padded);

  int ijk[3], source[3];
  vtkPointData* inPD = grid->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  outPD->CopyAllocate(inPD, output->GetNumberOfPoints());
  for (ijk[2] = padded[4]; ijk[2] <= padded[5]; ++ijk[2])
  {
    for (ijk[1] = padded[2]; ijk[1] <= padded[3]; ++ijk[1])
    {
      for (ijk[0] = padded[0]; ijk[0] <= padded[1]; ++ijk[0])
      {
        for (int d = 0; d < 3; ++d)
        {
          source[d] = std::clamp(ijk[d], extent[2 * d], extent[2 * d + 1]);
        }
        outPD->CopyData(inPD, vtkStructuredData::ComputePointIdForExtent(extent, source),
          vtkStructuredData::ComputePointIdForExtent(padded, ijk));
      }
    }
  }

  vtkCellData* inCD = grid->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  outCD->CopyAllocate(inCD, output->GetNumberOfCells());
  vtkUnsignedCharArray* inGhosts = grid->GetCellGhostArray();
  vtkNew<vtkUnsignedCharArray> ghosts;
  ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
  ghosts->SetNumberOfValues(output->GetNumberOfCells());
  for (ijk[2] = padded[4]; ijk[2] < padded[5]; ++ijk[2])
  {
    for (ijk[1] = padded[2]; ijk[1] < padded[3]; ++ijk[1])
    {
      for (ijk[0] = padded[0]; ijk[0] < padded[1]; ++ijk[0])
      {
        bool ghost = false;
        for (int d = 0; d < 3; ++d)
        {
          source[d] = std::clamp(ijk[d], extent[2 * d], extent[2 * d + 1] - 1);
          ghost |= source[d] != ijk[d];
        }
        const vtkIdType cellId = vtkStructuredData::ComputeCellIdForExtent(padded, ijk);
        const vtkIdType sourceId = vtkStructuredData::ComputeCellIdForExtent(extent, source);
        outCD->CopyData(inCD, sourceId, cellId);
        // Ghosts keep the other bits of the cell they copy, such as the blanking of refined cells.
        const unsigned char bits = inGhosts ? inGhosts->GetValue(sourceId) : 0;
        ghosts->SetValue(cellId, ghost ? (bits | vtkDataSetAttributes::DUPLICATECELL) : bits);
      }
    }
  }
  outCD->AddArray(ghosts);
  return output;
}

//------------------------------------------------------------------------------
// Ghost layers around the grids must not change the interface.
bool TestGhostLayers()
{
  vtkSmartPointer<vtkOverlappingAMR> plain = ::MakePulse();
  vtkSmartPointer<vtkOverlappingAMR> padded = ::MakePulse();
  const unsigned int numberOfBlocks = padded->GetNumberOfBlocks();
  for (unsigned int blockIdx = 0; blockIdx < numberOfBlocks; ++blockIdx)
  {
    unsigned int level, gridIdx;
    padded->ComputeIndexPair(blockIdx, level, gridIdx);
    padded->SetDataSet(level, gridIdx,
      ::AddGhostLayer(
        vtkUniformGrid::SafeDownCast(padded->GetDataSetAsCartesianGrid(level, gridIdx))));
  }

  vtkSmartPointer<vtkPartitionedDataSet> expected = ::ComputeInterface(plain, nullptr);
  vtkSmartPointer<vtkPartitionedDataSet> result = ::ComputeInterface(padded, nullptr);
  if (expected->GetNumberOfPartitions() != result->GetNumberOfPartitions())
  {
    vtkLog(ERROR, "Ghost layers changed the number of output partitions");
    return false;
  }
  for (unsigned int partitionIdx = 0; partitionIdx < expected->GetNumberOfPartitions();
       ++partitionIdx)
  {
    vtkDataSet* expectedPartition = expected->GetPartition(partitionIdx);
    vtkDataSet* resultPartition = result->GetPartition(partitionIdx);
    if (vtkUnstructuredGrid::SafeDownCast(expectedPartition))
    {
      if (!vtkTestUtilities::CompareDataObjects(expectedPartition, resultPartition))
      {
        vtkLog(ERROR, "Ghost layers changed interface " << partitionIdx);
        return false;
      }
    }
    else if (expectedPartition->GetNumberOfCells() != resultPartition->GetNumberOfCells())
    {
      vtkLog(ERROR, "The ghost layers of grid " << partitionIdx << " were not removed");
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
// Each process holds only the grids it owns, as for a distributed AMR: the hierarchy is global and
// the grids of the other processes are null. Every process must build the same interface for its
// grids as a single process holding them all.
bool TestDistributedInterface(
  vtkMultiProcessController* controller, vtkSmartPointer<vtkOverlappingAMR> (*makeAMR)())
{
  const int rank = controller->GetLocalProcessId();
  const int numberOfProcesses = controller->GetNumberOfProcesses();

  vtkSmartPointer<vtkOverlappingAMR> full = makeAMR();
  const unsigned int numberOfBlocks = full->GetNumberOfBlocks();
  std::vector<unsigned int> blocksPerLevel(full->GetNumberOfLevels());
  for (unsigned int level = 0; level < full->GetNumberOfLevels(); ++level)
  {
    blocksPerLevel[level] = full->GetNumberOfBlocks(level);
  }
  vtkNew<vtkOverlappingAMR> distributed;
  distributed->Initialize(blocksPerLevel);
  distributed->SetOrigin(full->GetOrigin());
  distributed->SetGridDescription(full->GetGridDescription());
  for (unsigned int level = 0; level < full->GetNumberOfLevels(); ++level)
  {
    double spacing[3];
    full->GetSpacing(level, spacing);
    distributed->SetSpacing(level, spacing);
    distributed->SetRefinementRatio(level, full->GetRefinementRatio(level));
  }
  for (unsigned int blockIdx = 0; blockIdx < numberOfBlocks; ++blockIdx)
  {
    unsigned int level, gridIdx;
    full->ComputeIndexPair(blockIdx, level, gridIdx);
    distributed->SetAMRBox(level, gridIdx, full->GetAMRBox(level, gridIdx));
    if (static_cast<int>(blockIdx) % numberOfProcesses == rank)
    {
      distributed->SetDataSet(level, gridIdx, full->GetDataSetAsCartesianGrid(level, gridIdx));
    }
  }

  vtkSmartPointer<vtkPartitionedDataSet> expected = ::ComputeInterface(makeAMR(), nullptr);
  vtkSmartPointer<vtkPartitionedDataSet> result = ::ComputeInterface(distributed, controller);

  const std::vector<vtkUnstructuredGrid*> expectedInterfaces = ::GetInterfaces(expected);
  const std::vector<vtkUnstructuredGrid*> interfaces = ::GetInterfaces(result);

  bool success = true;
  if (expectedInterfaces.empty())
  {
    vtkLog(ERROR, "The test AMR has no interface");
    success = false;
  }
  for (vtkUnstructuredGrid* interface : interfaces)
  {
    // The interfaces of different blocks differ in size or bounds, so only the one with the same
    // size and bounds is compared, which keeps the comparison of the others from being logged.
    auto sameSizeAndBounds = [interface](vtkUnstructuredGrid* expectedInterface)
    {
      if (expectedInterface->GetNumberOfPoints() != interface->GetNumberOfPoints() ||
        expectedInterface->GetNumberOfCells() != interface->GetNumberOfCells())
      {
        return false;
      }
      double expectedBounds[6], bounds[6];
      expectedInterface->GetBounds(expectedBounds);
      interface->GetBounds(bounds);
      return std::equal(expectedBounds, expectedBounds + 6, bounds,
        [](double a, double b) { return vtkMathUtilities::FuzzyCompare(a, b); });
    };
    auto candidate =
      std::find_if(expectedInterfaces.begin(), expectedInterfaces.end(), sameSizeAndBounds);
    if (candidate == expectedInterfaces.end() ||
      !vtkTestUtilities::CompareDataObjects(*candidate, interface))
    {
      vtkLog(ERROR, "Process " << rank << " built an interface that a single process does not");
      success = false;
    }
  }

  int numberOfInterfaces = static_cast<int>(interfaces.size());
  int totalNumberOfInterfaces = 0;
  controller->AllReduce(&numberOfInterfaces, &totalNumberOfInterfaces, 1, vtkCommunicator::SUM_OP);
  if (totalNumberOfInterfaces != static_cast<int>(expectedInterfaces.size()))
  {
    vtkLog(ERROR,
      "The processes built " << totalNumberOfInterfaces << " interfaces instead of "
                             << expectedInterfaces.size());
    success = false;
  }
  return success;
}
}

//------------------------------------------------------------------------------
int TestAMRInterfaceDistributed(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  int success = ::TestGhostLayers();
  success = ::TestDistributedInterface(controller, ::MakePulse) && success;
  success = ::TestDistributedInterface(controller, ::MakeManyBlocks) && success;

  int allSuccess = 0;
  controller->AllReduce(&success, &allSuccess, 1, vtkCommunicator::MIN_OP);

  vtkMultiProcessController::SetGlobalController(nullptr);
  controller->Finalize();
  return allSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
}
