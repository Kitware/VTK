// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkExtentTranslator.h"
#include "vtkGenerateGlobalIds.h"
#include "vtkIdTypeArray.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkStructuredData.h"
#include "vtkUnsignedCharArray.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include <algorithm>
#include <limits>
#include <vector>

namespace
{
int whole_extent[] = { 0, 99, 0, 99, 0, 99 };

vtkSmartPointer<vtkMultiBlockDataSet> CreateDataSet(
  vtkMultiProcessController* contr, int ghost_level, int nblocks)
{
  const int num_ranks = contr ? contr->GetNumberOfProcesses() : 1;
  const int rank = contr ? contr->GetLocalProcessId() : 0;
  const int total_nblocks = nblocks * num_ranks;

  vtkNew<vtkExtentTranslator> translator;
  translator->SetWholeExtent(whole_extent);
  translator->SetNumberOfPieces(total_nblocks);
  translator->SetGhostLevel(ghost_level);

  vtkNew<vtkMultiBlockDataSet> mb;
  for (int cc = 0; cc < nblocks; ++cc)
  {
    translator->SetPiece(rank * nblocks + cc);
    translator->PieceToExtent();

    int ext[6];
    translator->GetExtent(ext);

    vtkNew<vtkRTAnalyticSource> source;
    source->SetWholeExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
    source->Update();

    mb->SetBlock(rank * nblocks + cc, source->GetOutputDataObject(0));
  }
  return mb;
}

bool ValidateDataset(vtkMultiBlockDataSet* mb, vtkMultiProcessController* contr,
  int vtkNotUsed(ghost_level), int nblocks)
{
  const int num_ranks = contr ? contr->GetNumberOfProcesses() : 1;
  const int total_nblocks = nblocks * num_ranks;

  vtkIdType local_non_duplicated_points = 0;
  vtkIdType local_ptid_max = 0;
  vtkIdType local_cellid_max = 0;
  for (int cc = 0; cc < total_nblocks; ++cc)
  {
    if (auto ds = vtkDataSet::SafeDownCast(mb->GetBlock(cc)))
    {
      if (auto gpoints = vtkUnsignedCharArray::SafeDownCast(
            ds->GetPointData()->GetArray(vtkDataSetAttributes::GhostArrayName())))
      {
        for (vtkIdType kk = 0, max = gpoints->GetNumberOfTuples(); kk < max; ++kk)
        {
          local_non_duplicated_points += (gpoints->GetTypedComponent(kk, 0) == 0) ? 1 : 0;
        }
      }
      if (auto gpids = vtkIdTypeArray::SafeDownCast(ds->GetPointData()->GetGlobalIds()))
      {
        local_ptid_max = std::max(static_cast<vtkIdType>(gpids->GetRange(0)[1]), local_ptid_max);
      }
      if (auto gcids = vtkIdTypeArray::SafeDownCast(ds->GetCellData()->GetGlobalIds()))
      {
        local_cellid_max =
          std::max(static_cast<vtkIdType>(gcids->GetRange(0)[1]), local_cellid_max);
      }
    }
  }

  vtkIdType global_non_duplicated_points;
  contr->AllReduce(
    &local_non_duplicated_points, &global_non_duplicated_points, 1, vtkCommunicator::SUM_OP);
  if (global_non_duplicated_points != vtkStructuredData::GetNumberOfPoints(whole_extent))
  {
    vtkLogF(ERROR, "incorrect non-duplicated points; ghost points marked incorrectly!");
    return false;
  }

  vtkIdType global_ptid_max;
  contr->AllReduce(&local_ptid_max, &global_ptid_max, 1, vtkCommunicator::MAX_OP);
  const vtkIdType expected_ptid_max = vtkStructuredData::GetNumberOfPoints(whole_extent) - 1;
  if (global_ptid_max != expected_ptid_max)
  {
    vtkLogF(
      ERROR, "incorrect global point ids! %d, %d", (int)global_ptid_max, (int)expected_ptid_max);
    return false;
  }

  vtkIdType global_cellid_max;
  contr->AllReduce(&local_cellid_max, &global_cellid_max, 1, vtkCommunicator::MAX_OP);
  if (global_cellid_max != vtkStructuredData::GetNumberOfCells(whole_extent) - 1)
  {
    vtkLogF(ERROR, "incorrect global cell ids!");
    return false;
  }

  return true;
}

bool ValidateOutputHiddenGhosts(vtkUnsignedCharArray* ghosts)
{
  vtkIdType numberOfGhosts = 0;
  for (vtkIdType id = 0; id < ghosts->GetNumberOfValues(); ++id)
  {
    numberOfGhosts += (ghosts->GetValue(id) & vtkDataSetAttributes::HIDDENPOINT) != 0;
  }

  if (numberOfGhosts != ghosts->GetNumberOfValues() / 2)
  {
    vtkLogF(ERROR, "incorrect hidden ghosts in output");
    return false;
  }

  return true;
}

// A plane of 9x9 quads in 27 partitions of 3x1 quads, partition p on rank p % ranks.
vtkSmartPointer<vtkPartitionedDataSet> CreateFlatQuads(vtkMultiProcessController* contr)
{
  const int num_ranks = contr->GetNumberOfProcesses();
  const int rank = contr->GetLocalProcessId();
  auto pds = vtkSmartPointer<vtkPartitionedDataSet>::New();
  for (int partition = rank; partition < 27; partition += num_ranks)
  {
    const int i0 = 3 * (partition % 3);
    const int j0 = partition / 3;
    vtkNew<vtkPoints> points;
    for (int j = j0; j <= j0 + 1; ++j)
    {
      for (int i = i0; i <= i0 + 3; ++i)
      {
        points->InsertNextPoint(i, j, 0.0);
      }
    }
    vtkNew<vtkCellArray> polys;
    for (vtkIdType i = 0; i < 3; ++i)
    {
      polys->InsertNextCell({ i, i + 1, i + 5, i + 4 });
    }
    vtkNew<vtkPolyData> polyData;
    polyData->SetPoints(points);
    polyData->SetPolys(polys);
    pds->SetPartition(pds->GetNumberOfPartitions(), polyData);
  }
  return pds;
}

// Whether each of the n positions has one id, the same in all partitions (its minimum and maximum
// ids are equal), and whether these ids are 0 to n - 1.
bool IsPermutation(const std::vector<vtkIdType>& minIds, const std::vector<vtkIdType>& maxIds)
{
  const vtkIdType n = static_cast<vtkIdType>(minIds.size());
  std::vector<bool> used(n, false);
  for (vtkIdType position = 0; position < n; ++position)
  {
    const vtkIdType id = minIds[position];
    if (id != maxIds[position] || id < 0 || id >= n || used[id])
    {
      return false;
    }
    used[id] = true;
  }
  return true;
}

// The 100 points and 81 quads of CreateFlatQuads, numbered by position, have global ids 0 to 99
// and 0 to 80, and a point shared by several partitions has the same id in all of them.
bool ValidateFlatQuads(vtkPartitionedDataSet* pds, vtkMultiProcessController* contr)
{
  constexpr vtkIdType unseen = std::numeric_limits<vtkIdType>::max();
  std::vector<vtkIdType> localMinIds(100 + 81, unseen);
  std::vector<vtkIdType> localMaxIds(100 + 81, -1);
  for (unsigned int cc = 0; cc < pds->GetNumberOfPartitions(); ++cc)
  {
    vtkPolyData* polyData = vtkPolyData::SafeDownCast(pds->GetPartition(cc));
    auto gpids = vtkIdTypeArray::SafeDownCast(polyData->GetPointData()->GetGlobalIds());
    auto gcids = vtkIdTypeArray::SafeDownCast(polyData->GetCellData()->GetGlobalIds());
    if (!gpids || !gcids)
    {
      vtkLogF(ERROR, "missing global ids on the flat quads!");
      return false;
    }
    for (vtkIdType pointId = 0; pointId < polyData->GetNumberOfPoints(); ++pointId)
    {
      double p[3];
      polyData->GetPoint(pointId, p);
      const vtkIdType position = static_cast<vtkIdType>(p[0]) + 10 * static_cast<vtkIdType>(p[1]);
      localMinIds[position] = std::min(localMinIds[position], gpids->GetValue(pointId));
      localMaxIds[position] = std::max(localMaxIds[position], gpids->GetValue(pointId));
    }
    for (vtkIdType cellId = 0; cellId < polyData->GetNumberOfCells(); ++cellId)
    {
      // The first point of a quad is its lower left corner.
      double p[3];
      polyData->GetPoint(polyData->GetPolys()->GetCellPointAtId(cellId, 0), p);
      const vtkIdType position =
        100 + static_cast<vtkIdType>(p[0]) + 9 * static_cast<vtkIdType>(p[1]);
      localMinIds[position] = std::min(localMinIds[position], gcids->GetValue(cellId));
      localMaxIds[position] = std::max(localMaxIds[position], gcids->GetValue(cellId));
    }
  }

  std::vector<vtkIdType> minIds(localMinIds.size());
  std::vector<vtkIdType> maxIds(localMaxIds.size());
  contr->AllReduce(localMinIds.data(), minIds.data(), static_cast<vtkIdType>(minIds.size()),
    vtkCommunicator::MIN_OP);
  contr->AllReduce(localMaxIds.data(), maxIds.data(), static_cast<vtkIdType>(maxIds.size()),
    vtkCommunicator::MAX_OP);
  if (!IsPermutation(
        { minIds.begin(), minIds.begin() + 100 }, { maxIds.begin(), maxIds.begin() + 100 }) ||
    !IsPermutation({ minIds.begin() + 100, minIds.end() }, { maxIds.begin() + 100, maxIds.end() }))
  {
    vtkLogF(ERROR, "incorrect global ids on the flat quads!");
    return false;
  }
  return true;
}
}

int TestGenerateGlobalIds(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkMPIController* contr = vtkMPIController::New();
#else
  vtkDummyController* contr = vtkDummyController::New();
#endif
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  int status = EXIT_SUCCESS;
  if (auto dataset = CreateDataSet(contr, 0, 3)) // no cell overlap
  {
    vtkNew<vtkGenerateGlobalIds> generator;
    generator->SetInputDataObject(dataset);
    generator->Update();

    if (!ValidateDataset(
          vtkMultiBlockDataSet::SafeDownCast(generator->GetOutputDataObject(0)), contr, 0, 3))
    {
      status = EXIT_FAILURE;
    }
  }

  if (auto dataset = CreateDataSet(contr, 1, 3)) // cell overlap
  {
    vtkNew<vtkGenerateGlobalIds> generator;
    generator->SetInputDataObject(dataset);
    generator->Update();

    if (!ValidateDataset(
          vtkMultiBlockDataSet::SafeDownCast(generator->GetOutputDataObject(0)), contr, 1, 3))
    {
      status = EXIT_FAILURE;
    }
  }

  // test a dataset with 1 block per rank.
  if (auto dataset = CreateDataSet(contr, 1, 1))
  {
    {
      vtkNew<vtkGenerateGlobalIds> generator;
      generator->SetInputDataObject(dataset);
      generator->Update();

      if (!ValidateDataset(
            vtkMultiBlockDataSet::SafeDownCast(generator->GetOutputDataObject(0)), contr, 1, 1))
      {
        status = EXIT_FAILURE;
      }
    }

    {
      int rank = contr->GetLocalProcessId();
      vtkNew<vtkUnsignedCharArray> inputGhosts;
      inputGhosts->SetName(vtkDataSetAttributes::GhostArrayName());
      auto block = vtkDataSet::SafeDownCast(dataset->GetBlock(rank));
      inputGhosts->SetNumberOfValues(block->GetNumberOfPoints());
      for (vtkIdType pointId = 0; pointId < inputGhosts->GetNumberOfValues(); ++pointId)
      {
        inputGhosts->SetValue(pointId, pointId % 2 ? 0 : vtkDataSetAttributes::HIDDENPOINT);
      }
      block->GetPointData()->AddArray(inputGhosts);
      vtkNew<vtkGenerateGlobalIds> generator;
      generator->SetInputDataObject(dataset);
      generator->Update();

      auto outMB = vtkMultiBlockDataSet::SafeDownCast(generator->GetOutputDataObject(0));
      auto outDS = vtkDataSet::SafeDownCast(outMB->GetBlock(rank));
      if (!ValidateOutputHiddenGhosts(outDS->GetPointData()->GetGhostArray()))
      {
        status = EXIT_FAILURE;
      }
    }
  }

  // test a flat dataset, whose bounds have no thickness in z.
  {
    vtkNew<vtkGenerateGlobalIds> generator;
    generator->SetInputDataObject(CreateFlatQuads(contr));
    generator->Update();

    if (!ValidateFlatQuads(
          vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0)), contr))
    {
      status = EXIT_FAILURE;
    }
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  contr->Delete();
  return status;
}
