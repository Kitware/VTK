// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPCorrelativeStatistics.h"

#include "vtkCompositeDataSet.h"
#include "vtkCorrelativeStatistics.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMathUtilities.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <math.h>

namespace
{
//------------------------------------------------------------------------------
// If rank == -1, we generate all samples
vtkSmartPointer<vtkTable> GenerateTable(int rank, int numberOfRanks, vtkIdType N)
{
  // We will generate a function looking like that:
  // 1: a * sin(exp(-lambda * x)) + b * x
  // 2: sin(x)

  vtkIdType begin = (N / numberOfRanks) * rank;
  vtkIdType end = rank == numberOfRanks - 1 ? N : begin + N / numberOfRanks;

  if (rank == -1)
  {
    begin = 0;
    end = N;
  }

  auto table = vtkSmartPointer<vtkTable>::New();

  vtkNew<vtkDoubleArray> array1, array2;
  array1->SetName("Array 1");
  array1->SetNumberOfValues(end - begin);
  array2->SetName("Array 2");
  array2->SetNumberOfValues(end - begin);

  table->AddColumn(array1);
  table->AddColumn(array2);

  double a = 1.0, b = 1.0, lambda = 1.0;

  for (vtkIdType id = begin; id < end; ++id)
  {
    double x = static_cast<double>(id) / N;

    array1->SetValue(id - begin, a * std::sin(std::exp(-lambda * x)) + b * x);
    array2->SetValue(id - begin, std::sin(x));
  }

  return table;
}

//------------------------------------------------------------------------------
bool TablesAreSame(vtkTable* table, vtkTable* ref, vtkIdType offset = 0)
{
  for (vtkIdType columnId = 0; columnId < ref->GetNumberOfColumns(); ++columnId)
  {
    vtkDataArray* array = vtkArrayDownCast<vtkDoubleArray>(table->GetColumn(columnId));
    vtkDataArray* refArray = vtkArrayDownCast<vtkDoubleArray>(ref->GetColumn(columnId));

    if (!refArray)
    {
      continue;
    }

    for (vtkIdType id = offset; id < table->GetNumberOfRows() + offset; ++id)
    {
      if (std::fabs(array->GetTuple1(id - offset) - refArray->GetTuple1(id)) > 1e-6)
      {
        return false;
      }
    }
  }

  return true;
}
} // anonymous namespace

//------------------------------------------------------------------------------
int TestPCorrelativeStatistics(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  int myrank = controller->GetLocalProcessId();
  int numberOfRanks = controller->GetNumberOfProcesses();
  int retVal = EXIT_SUCCESS;

  vtkIdType N = 100;

  vtkSmartPointer<vtkTable> table = GenerateTable(myrank, numberOfRanks, N);

  vtkNew<vtkPCorrelativeStatistics> stats;
  stats->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, table);
  stats->AddColumnPair("Array 1", "Array 2");
  stats->SetLearnOption(true);
  stats->SetDeriveOption(true);
  stats->SetAssessOption(true);
  stats->SetTestOption(false);
  stats->Update();

  vtkSmartPointer<vtkTable> refTable = GenerateTable(-1, numberOfRanks, N);

  vtkNew<vtkCorrelativeStatistics> refStats;
  refStats->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, refTable);
  refStats->AddColumnPair("Array 1", "Array 2");
  refStats->SetLearnOption(true);
  refStats->SetDeriveOption(true);
  refStats->SetAssessOption(true);
  refStats->SetTestOption(false);
  refStats->Update();

  auto outData = vtkTable::SafeDownCast(stats->GetOutputDataObject(0));
  auto outModel = vtkMultiBlockDataSet::SafeDownCast(stats->GetOutputDataObject(1));
  auto outTests = vtkTable::SafeDownCast(stats->GetOutputDataObject(2));

  auto outRefData = vtkTable::SafeDownCast(refStats->GetOutputDataObject(0));
  auto outRefModel = vtkMultiBlockDataSet::SafeDownCast(refStats->GetOutputDataObject(1));
  auto outRefTests = vtkTable::SafeDownCast(refStats->GetOutputDataObject(2));

  auto outPrimaryTable = vtkTable::SafeDownCast(outModel->GetBlock(0));
  auto outRefPrimaryTable = vtkTable::SafeDownCast(outRefModel->GetBlock(0));

  if (!TablesAreSame(outPrimaryTable, outRefPrimaryTable))
  {
    vtkLog(ERROR, "Measured statistics mismatch between single-process and multi-process.");
    retVal = EXIT_FAILURE;
  }

  // Testing outData.
  // There should be a copy of the input in the first 2 columns, and the result of assessing.
  if (!TablesAreSame(outData, outRefData, myrank == 0 ? 0 : N / 2))
  {
    vtkLog(ERROR, "Assessing statistics failed");
    retVal = EXIT_FAILURE;
  }

  // Testing outTests
  if (!TablesAreSame(outTests, outRefTests))
  {
    vtkLog(ERROR, "Testing statistics failed");
    retVal = EXIT_FAILURE;
  }

  controller->Finalize();

  return retVal;
}
