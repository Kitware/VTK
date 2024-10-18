// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGenerateGlobalIds.h"
#include "vtkHyperTreeGridGhostCellsGenerator.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLHyperTreeGridReader.h"

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

  const int expectedNbOfCells[4] = { 224, 312, 200, 280 };

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
    int expectedGhostType = (i < nbCellsBefore) ? 0 : 1;
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

/**
 * Test with a simple 2D case
 */
int TestGhost2D(vtkMPIController* controller)
{
  int ret = EXIT_SUCCESS;

  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  // Setup pipeline
  vtkNew<vtkRandomHyperTreeGridSource> htgSource;
  htgSource->SetSeed(0);
  htgSource->SetMaxDepth(2);
  htgSource->SetDimensions(5, 5, 5);
  htgSource->SetMaskedFraction(0.3);
  htgSource->SetSplitFraction(0.3);
  htgSource->UpdatePiece(myRank, nbRanks, 0);
  int nbCellsBefore = htgSource->GetHyperTreeGridOutput()->GetNumberOfCells();
  vtkLog(TRACE, << "number of cells (before Generator): " << nbCellsBefore);

  // Create GCG
  vtkHyperTreeGridGhostCellsGenerator* generator =
    vtkHyperTreeGridGhostCellsGenerator::New(); // early delete
  generator->SetInputConnection(htgSource->GetOutputPort());
  vtkSmartPointer<vtkHyperTreeGrid> htg(generator->GetHyperTreeGridOutput());
  if (generator->UpdatePiece(myRank, nbRanks, 0) != 1)
  {
    vtkErrorWithObjectMacro(nullptr, << "Fail to update piece for process " << myRank);
    return EXIT_FAILURE;
  }

  generator->Delete(); // check the cell data are still consistent

  auto nbCellsAfter = htg->GetNumberOfCells();
  vtkLog(TRACE, << "number of cells (after Generator): " << nbCellsAfter);

  htg->GetCellData()->GetArray("Depth")->GetTuple1(nbCellsAfter - 3);

  return ret;
}

/**
 * With one or more partitions not containing cells, ghost cells should still be generated
 */
int TestGhostNullPart(vtkMPIController* controller)
{
  int ret = EXIT_SUCCESS;

  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  // Setup HTG Source: process 1, 2 and 3 have cells, the others do not.
  // Still, GCG should generate ghost cells, except for process 0
  /**
   * The distributed HTG with process ids looks like this
   * +---+---+---+
   * | 1 | 1 | 1 |
   * +---+---+---+
   * | 1 | 1 | 1 |
   * +---+---+---+
   * | 2 | 2 | 2 |
   * |---+---+---+
   * | 2 | 2 | 2 |
   * |---+---+---+
   * | 3 | 3 | 3 |
   * |---+---+---+
   *
   * It should have ghost cells:
   * +---+---+---+
   * |   |   |   |
   * +---+---+---+
   * | 2 | 2 | 2 |
   * +---+---+---+
   * | 1 | 1 | 1 |
   * |---+---+---+
   * | 3 | 3 | 3 |
   * |---+---+---+
   * | 2 | 2 | 2 |
   * |---+---+---+
   */

  vtkNew<vtkHyperTreeGridSource> htgSource;
  htgSource->SetDimensions(4, 6, 1);

  htgSource->SetDescriptor("1... ... 2... ... 3...");
  htgSource->SetMaxDepth(1);
  htgSource->SetUseMask(false);

  // Create GCG
  vtkNew<vtkHyperTreeGridGhostCellsGenerator> generator;
  generator->SetDebug(true);
  generator->SetInputConnection(htgSource->GetOutputPort());
  vtkSmartPointer<vtkHyperTreeGrid> htg(generator->GetHyperTreeGridOutput());
  if (generator->UpdatePiece(myRank, nbRanks, 0) != 1)
  {
    vtkErrorWithObjectMacro(nullptr, << "Fail to update piece for process " << myRank);
    return EXIT_FAILURE;
  }

  std::array<vtkIdType, 4> expectedNbOfCells{ 0, 9, 12, 6 };
  vtkIdType nbCellsAfter = htg->GetNumberOfCells();
  if (expectedNbOfCells[myRank] != nbCellsAfter)
  {
    vtkErrorWithObjectMacro(nullptr, << "Wrong number of ghost cells generated for process "
                                     << myRank << ". Has " << nbCellsAfter << " but expect "
                                     << expectedNbOfCells[myRank]);
    ret = EXIT_FAILURE;
  }

  // Now, only one process has cells, so no ghost cells should be generated
  htgSource->SetDescriptor("2... ... ... ... ...");
  if (generator->UpdatePiece(myRank, nbRanks, 0) != 1)
  {
    vtkErrorWithObjectMacro(nullptr, << "Fail to update piece for process " << myRank);
    return EXIT_FAILURE;
  }

  std::array<vtkIdType, 4> expectedNbOfCells_1Process{ 0, 0, 15, 0 };
  nbCellsAfter = htg->GetNumberOfCells();
  if (expectedNbOfCells_1Process[myRank] != nbCellsAfter)
  {
    vtkErrorWithObjectMacro(nullptr, << "Wrong number of ghost cells generated for process "
                                     << myRank << ". Has " << nbCellsAfter << " but expect "
                                     << expectedNbOfCells[myRank]);
    ret = EXIT_FAILURE;
  }

  return ret;
}

/**
 * Make sure the Ghost Cells filter behaves correctly when given a non-distributed input, such as a
 * HTG read from a .htg file. No ghost cells should be generated in that case
 */
int TestGhostSinglePiece(vtkMPIController* controller, const std::string& filename)
{
  int ret = EXIT_SUCCESS;

  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  // Read HTG From file
  vtkNew<vtkXMLHyperTreeGridReader> reader;
  reader->SetFileName(filename.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);

  // Create GCG
  vtkNew<vtkHyperTreeGridGhostCellsGenerator> generator;
  generator->SetDebug(true);
  generator->SetInputConnection(reader->GetOutputPort());
  vtkSmartPointer<vtkHyperTreeGrid> htgGhosted(generator->GetHyperTreeGridOutput());
  vtkSmartPointer<vtkHyperTreeGrid> htgRead(
    vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0)));
  if (generator->UpdatePiece(myRank, nbRanks, 0) != 1)
  {
    vtkErrorWithObjectMacro(nullptr, << "Fail to update piece for process " << myRank);
    return EXIT_FAILURE;
  }

  vtkIdType nbCellsBefore = htgRead->GetNumberOfCells();
  vtkIdType nbCellsAfter = htgGhosted->GetNumberOfCells();
  if (nbCellsAfter != nbCellsBefore)
  {
    vtkErrorWithObjectMacro(nullptr, << "Wrong number of ghost cells generated for process "
                                     << myRank << ". Has " << nbCellsAfter << " but expect "
                                     << nbCellsBefore);
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

  std::string htgFileName{ vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMR/htg3d.htg") };

  // Run actual tests
  ret |= ::TestGhostCellFields(controller);
  ret |= ::TestGhostMasking(controller);
  ret |= ::TestGhost2D(controller);
  ret |= ::TestGhostNullPart(controller);
  ret |= ::TestGhostSinglePiece(controller, htgFileName);

  controller->Finalize();
  return ret;
}
