// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGhostCellsGenerator.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkUnsignedCharArray.h"

#include "vtkLogger.h"

namespace
{
/**
 * For a given non-null array, return 0 if the number of components, tuples and range match with the
 * arguments, and return 1 otherwise.
 */
int CheckArray(
  vtkDataArray* array, int numberComponents, int numberTuples, const double* range, int rank)
{
  int ret = EXIT_SUCCESS;
  if (!array)
  {
    vtkErrorWithObjectMacro(nullptr, << "Array could not be found");
    return EXIT_FAILURE;
  }
  if (array->GetNumberOfComponents() != numberComponents)
  {
    vtkErrorWithObjectMacro(
      nullptr, << "Wrong number of components in the scalar cell field for process " << rank
               << ". Has " << array->GetNumberOfComponents() << " but expect " << numberComponents);
    ret = EXIT_FAILURE;
  }
  if (array->GetNumberOfTuples() != numberTuples)
  {
    vtkErrorWithObjectMacro(
      nullptr, << "Wrong number of tuples in the scalar cell field for process " << rank << ". Has "
               << array->GetNumberOfTuples() << " but expect " << numberTuples);
    ret = EXIT_FAILURE;
  }
  if (array->GetRange()[0] != range[0] || array->GetRange()[1] != range[1])
  {
    vtkErrorWithObjectMacro(nullptr, << "Wrong range for the the scalar cell field for process "
                                     << rank << ". Got [" << array->GetRange()[0] << ","
                                     << array->GetRange()[1] << "] but expected [" << range[0]
                                     << "," << range[1] << "]");
    ret = EXIT_FAILURE;
  }
  return ret;
}
}

// Program main
int TestHyperTreeGridGhostCellsGenerator(int argc, char* argv[])
{
  int ret = EXIT_SUCCESS;
  // Initialize MPI
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(controller);
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();
  const int expectedNbOfCells[4] = { 352, 408, 344, 464 };
  const double expectedScalarRange[2] = { 0, 30257 };

  // Initialize log
  std::string threadName = "rank #";
  threadName += std::to_string(myRank);
  vtkLogger::SetThreadName(threadName);

  // Setup pipeline
  vtkNew<vtkRandomHyperTreeGridSource> htgSource;
  htgSource->SetSeed(3);
  htgSource->SetMaxDepth(3);
  htgSource->SetDimensions(3, 3, 3);
  htgSource->SetMaskedFraction(0.0);
  htgSource->UpdatePiece(myRank, nbRanks, 0);
  int nbCells = htgSource->GetHyperTreeGridOutput()->GetNumberOfCells();
  vtkLog(TRACE, << "number of cells (before Generator): " << nbCells);

  // Create cell fields
  vtkNew<vtkDoubleArray> scalarData;
  scalarData->SetNumberOfComponents(1);
  scalarData->SetNumberOfTuples(nbCells);
  scalarData->SetName("ScalarArray");

  vtkNew<vtkDoubleArray> vectorData;
  vectorData->SetNumberOfComponents(3);
  vectorData->SetNumberOfTuples(nbCells);
  vectorData->SetName("VectorArray");

  for (int i = 0; i < nbCells; i++)
  {
    vectorData->SetTuple3(
      i, 1.0 * i + myRank * 10000.0, 2.0 * i + myRank * 10000.0, 3.0 * i + myRank * 10000.0);
    scalarData->SetTuple1(i, 1.0 * i + myRank * 10000.0);
  }
  htgSource->GetHyperTreeGridOutput()->GetCellData()->SetScalars(scalarData);
  htgSource->GetHyperTreeGridOutput()->GetCellData()->SetVectors(vectorData);

  vtkNew<vtkHyperTreeGridGhostCellsGenerator> generator;
  generator->SetDebug(true);
  generator->SetInputConnection(htgSource->GetOutputPort());
  vtkHyperTreeGrid* htg = generator->GetHyperTreeGridOutput();
  if (generator->UpdatePiece(myRank, nbRanks, 0) != 1)
  {
    vtkErrorWithObjectMacro(nullptr, << "Fail to update piece");
    ret = EXIT_FAILURE;
  }
  auto nbOfCells = htg->GetNumberOfCells();
  vtkLog(TRACE, << "number of cells (after Generator): " << nbOfCells);

  // every piece should have some ghosts
  if (!(htg->HasAnyGhostCells() && htg->GetGhostCells()->GetNumberOfTuples() > 1))
  {
    vtkErrorWithObjectMacro(nullptr, << "No ghost cells generated.");
    ret = EXIT_FAILURE;
  }

  // This test is supposed to run on 4 nodes. In that case we can compare
  // with expected values
  if (nbRanks == 4)
  {
    if (expectedNbOfCells[myRank] != nbOfCells)
    {
      vtkErrorWithObjectMacro(nullptr, << "Wrong number of ghost cells generated for process "
                                       << myRank << ". Has " << nbOfCells << " but expect "
                                       << expectedNbOfCells[myRank]);
      ret = EXIT_FAILURE;
    }

    // Ghost cells should also have cell data values, transmitted by their neighbors
    vtkDataArray* outScalar = htg->GetCellData()->GetScalars("ScalarArray");
    if (::CheckArray(outScalar, 1, expectedNbOfCells[myRank], expectedScalarRange, myRank) ==
      EXIT_FAILURE)
    {
      vtkErrorWithObjectMacro(nullptr, << "Scalar array does not match");
      ret = EXIT_FAILURE;
    }

    vtkDataArray* outVector = htg->GetCellData()->GetVectors("VectorArray");
    if (::CheckArray(outVector, 3, expectedNbOfCells[myRank], expectedScalarRange, myRank) ==
      EXIT_FAILURE)
    {
      vtkErrorWithObjectMacro(nullptr, << "Vector array does not match");
      ret = EXIT_FAILURE;
    }
  }
  else
  {
    vtkLog(WARNING, << "test run on " << nbRanks << " ranks (4 expected). Cannot compare result");
  }

  controller->Finalize();
  return ret;
}
