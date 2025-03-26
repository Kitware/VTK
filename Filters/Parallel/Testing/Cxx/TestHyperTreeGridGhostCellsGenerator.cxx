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

/**
 * Test the transfer of scalar and vector arrays in ghost trees
 */
int TestGhostCellFields(vtkMPIController* controller)
{
  int ret = EXIT_SUCCESS;

  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  const int expectedNbOfCells[4] = { 336, 288, 408, 240 };
  const double expectedScalarRange[2] = { 0, 30001 };

  // Setup pipeline
  vtkNew<vtkRandomHyperTreeGridSource> htgSource;
  htgSource->SetSeed(3);
  htgSource->SetMaxDepth(3);
  htgSource->SetDimensions(3, 3, 3);
  htgSource->UpdatePiece(myRank, nbRanks, 0);
  int nbCellsBefore = htgSource->GetHyperTreeGridOutput()->GetNumberOfCells();
  vtkLog(TRACE, << "number of cells (before Generator): " << nbCellsBefore);

  // Create cell fields
  vtkNew<vtkDoubleArray> scalarData;
  scalarData->SetNumberOfComponents(1);
  scalarData->SetNumberOfTuples(nbCellsBefore);
  scalarData->SetName("ScalarArray");

  vtkNew<vtkDoubleArray> vectorData;
  vectorData->SetNumberOfComponents(3);
  vectorData->SetNumberOfTuples(nbCellsBefore);
  vectorData->SetName("VectorArray");

  for (int i = 0; i < nbCellsBefore; i++)
  {
    vectorData->SetTuple3(
      i, 1.0 * i + myRank * 10000.0, 2.0 * i + myRank * 10000.0, 3.0 * i + myRank * 10000.0);
    scalarData->SetTuple1(i, 1.0 * i + myRank * 10000.0);
  }
  htgSource->GetHyperTreeGridOutput()->GetCellData()->SetScalars(scalarData);
  htgSource->GetHyperTreeGridOutput()->GetCellData()->SetVectors(vectorData);
  htgSource->GetHyperTreeGridOutput()->SetMask(nullptr);

  vtkNew<vtkHyperTreeGridGhostCellsGenerator> generator;
  generator->SetInputConnection(htgSource->GetOutputPort());
  vtkHyperTreeGrid* htg = generator->GetHyperTreeGridOutput();
  if (generator->UpdatePiece(myRank, nbRanks, 0) != 1)
  {
    vtkErrorWithObjectMacro(nullptr, << "Fail to update piece for process " << myRank);
    return EXIT_FAILURE;
  }
  auto nbCellsAfter = htg->GetNumberOfCells();
  vtkLog(TRACE, << "number of cells (after Generator): " << nbCellsAfter);

  // every piece should have some ghosts
  if (!(htg->HasAnyGhostCells() && htg->GetGhostCells()->GetNumberOfTuples() > 1))
  {
    vtkErrorWithObjectMacro(nullptr, << "No ghost cells generated for process " << myRank);
    ret = EXIT_FAILURE;
  }

  if (expectedNbOfCells[myRank] != nbCellsAfter)
  {
    vtkErrorWithObjectMacro(nullptr, << "Wrong number of ghost cells generated for process "
                                     << myRank << ". Has " << nbCellsAfter << " but expect "
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

  return ret;
}

/**
 * Test the transfer of masked cells in ghost trees.
 */
int TestGhostMasking(vtkMPIController* controller)
{
  int ret = EXIT_SUCCESS;

  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  const int expectedNbOfCells[4] = { 208, 256, 192, 248 };
  const int expectedGhostTypeCutoff[4] = { 97, 145, 137, 89 };

  // Setup pipeline
  vtkNew<vtkRandomHyperTreeGridSource> htgSource;
  htgSource->SetSeed(1);
  htgSource->SetMaxDepth(3);
  htgSource->SetDimensions(3, 3, 3);
  htgSource->SetMaskedFraction(0.4);
  htgSource->UpdatePiece(myRank, nbRanks, 0);
  int nbCellsBefore = htgSource->GetHyperTreeGridOutput()->GetNumberOfCells();
  vtkLog(TRACE, << "number of cells (before Generator): " << nbCellsBefore);

  // Create GCG
  vtkNew<vtkHyperTreeGridGhostCellsGenerator> generator;
  generator->SetInputConnection(htgSource->GetOutputPort());
  vtkHyperTreeGrid* htg = generator->GetHyperTreeGridOutput();
  if (generator->UpdatePiece(myRank, nbRanks, 0) != 1)
  {
    vtkErrorWithObjectMacro(nullptr, << "Fail to update piece for process " << myRank);
    return EXIT_FAILURE;
  }

  auto nbCellsAfter = htg->GetNumberOfCells();
  vtkLog(TRACE, << "number of cells (after Generator): " << nbCellsAfter);

  // Verify the expected number of cells including ghosts
  if (expectedNbOfCells[myRank] != nbCellsAfter)
  {
    vtkErrorWithObjectMacro(nullptr, << "Wrong number of ghost cells generated for process "
                                     << myRank << ". Has " << nbCellsAfter << " but expect "
                                     << expectedNbOfCells[myRank]);
    ret = EXIT_FAILURE;
  }

  // Check that every piece has the right amount of ghost cells
  if (!htg->HasAnyGhostCells())
  {
    vtkErrorWithObjectMacro(nullptr, << "No ghost cells generated for process " << myRank);
    ret = EXIT_FAILURE;
  }
  for (int i = 0; i < expectedNbOfCells[myRank]; i++)
  {
    int expectedGhostType = (i <= expectedGhostTypeCutoff[myRank]) ? 0 : 1;
    if (htg->GetGhostCells()->GetTuple1(i) != expectedGhostType)
    {
      vtkErrorWithObjectMacro(nullptr, << "Expected ghost type " << expectedGhostType << " but got "
                                       << htg->GetGhostCells()->GetTuple1(i) << " for cell id " << i
                                       << " on process " << myRank);
      ret = EXIT_FAILURE;
    }
  }

  // Check that Depth cell data is correct
  const double expectedDepthRange[2] = { 0, 3 };
  vtkDataArray* outDepth = htg->GetCellData()->GetScalars("Depth");
  if (::CheckArray(outDepth, 1, expectedNbOfCells[myRank], expectedDepthRange, myRank) ==
    EXIT_FAILURE)
  {
    vtkErrorWithObjectMacro(
      nullptr, << "Depth array outside of expected range for rank " << myRank);
    ret = EXIT_FAILURE;
  }
  return ret;
}
}

/**
 * Subtest launcher, initializing MPI controller.
 */
int TestHyperTreeGridGhostCellsGenerator(int argc, char* argv[])
{
  int ret = EXIT_SUCCESS;

  // Initialize MPI
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(controller);

  // This test is supposed to run on 4 nodes. In that case we can compare
  // with expected values
  if (controller->GetNumberOfProcesses() != 4)
  {
    vtkLog(WARNING, << "test run on " << controller->GetNumberOfProcesses()
                    << " ranks (4 expected). Cannot compare result");
    return EXIT_FAILURE;
  }

  // Initialize log
  std::string threadName = "rank #";
  threadName += std::to_string(controller->GetLocalProcessId());
  vtkLogger::SetThreadName(threadName);

  // Run actual tests
  ret |= ::TestGhostCellFields(controller);
  ret |= ::TestGhostMasking(controller);

  controller->Finalize();
  return ret;
}
