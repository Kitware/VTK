#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGhostCellsGenerator.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkUnsignedCharArray.h"

#include "vtkLogger.h"

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
  const int expectedNbOfCells[2] = { 432, 480 };

  // Initialize log
  std::string threadName = "rank #";
  threadName += std::to_string(myRank);
  vtkLogger::SetThreadName(threadName);

  // Setup pipeline
  vtkNew<vtkRandomHyperTreeGridSource> htgSource;
  htgSource->SetSeed(1);
  htgSource->SetMaxDepth(3);
  htgSource->SetDimensions(3, 3, 3);
  htgSource->UpdatePiece(myRank, nbRanks, 0);
  vtkLog(TRACE, << "number of cells (before Generator): "
                << htgSource->GetHyperTreeGridOutput()->GetNumberOfCells());

  vtkNew<vtkHyperTreeGridGhostCellsGenerator> generator;
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

  // This test is supposed to run on 2 nodes. In that case we can compare
  // with expected values
  if (nbRanks == 2)
  {
    if (expectedNbOfCells[myRank] != nbOfCells)
    {
      vtkErrorWithObjectMacro(nullptr, << "Wrong number of ghost cells generated. Has " << nbOfCells
                                       << " but expect " << expectedNbOfCells[myRank]);
      ret = EXIT_FAILURE;
    }
  }
  else
  {
    vtkLog(WARNING, << "test run on " << nbRanks << " ranks (2 expected). Cannot compare result");
  }

  controller->Finalize();
  return ret;
}
