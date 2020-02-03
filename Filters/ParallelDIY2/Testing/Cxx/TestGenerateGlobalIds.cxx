/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenerateGlobalIds.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

===========================================================================*/

#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkDataSet.h"
#include "vtkExtentTranslator.h"
#include "vtkGenerateGlobalIds.h"
#include "vtkIdTypeArray.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkStructuredData.h"
#include "vtkUnsignedCharArray.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

namespace
{
static int whole_extent[] = { 0, 99, 0, 99, 0, 99 };

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
    vtkNew<vtkGenerateGlobalIds> generator;
    generator->SetInputDataObject(dataset);
    generator->Update();

    if (!ValidateDataset(
          vtkMultiBlockDataSet::SafeDownCast(generator->GetOutputDataObject(0)), contr, 1, 1))
    {
      status = EXIT_FAILURE;
    }
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  contr->Delete();
  return status;
}
