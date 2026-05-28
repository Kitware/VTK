#include "vtkGroupDataSetsFilter.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridExtractGhostCells.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkStringFormatter.h"

#include <array>

int TestGroupDataSetHTGGhostCellsGeneration(int argc, char* argv[])
{
  // Initialize MPI
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(controller);

  std::string threadName = "rank #";
  threadName += vtk::to_string(controller->GetLocalProcessId());
  vtkLogger::SetThreadName(threadName);

  // This test is supposed to run on 4 nodes. In that case we can compare
  // with expected values
  if (controller->GetNumberOfProcesses() != 4)
  {
    vtkLog(WARNING, << "test run on " << controller->GetNumberOfProcesses()
                    << " ranks (4 expected). Cannot compare result");
    return EXIT_FAILURE;
  }

  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  vtkNew<vtkRandomHyperTreeGridSource> htgSource;
  htgSource->SetSeed(3);
  htgSource->SetMaxDepth(3);
  htgSource->SetDimensions(5, 5, 5);

  // GroupDataSets triggers the executive for the request of a ghost cells level.
  // Since vtkRandomHyperTreeGridSource can produce a sub-extent, ghost cells are generated.
  vtkNew<vtkGroupDataSetsFilter> groupDataSetsFilter;
  groupDataSetsFilter->SetOutputTypeToMultiBlockDataSet();
  groupDataSetsFilter->AddInputConnection(htgSource->GetOutputPort());

  vtkNew<vtkHyperTreeGridExtractGhostCells> extractGhostCellsFilter;
  extractGhostCellsFilter->SetInputConnection(groupDataSetsFilter->GetOutputPort());
  extractGhostCellsFilter->UpdatePiece(myRank, nbRanks, 1);

  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(groupDataSetsFilter->GetOutput());
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(mb->GetBlock(0));

  std::array<vtkIdType, 4> expectedNumberOfCells = { 3180, 2996, 3428, 3428 };
  if (htg->GetNumberOfCells() != expectedNumberOfCells[myRank])
  {
    vtkLogF(ERROR, "Wrong number of ghost cells extracted. Expected %lld but got %lld",
      expectedNumberOfCells[myRank], htg->GetNumberOfCells());
    controller->Finalize();
    return EXIT_FAILURE;
  }

  controller->Finalize();
  return EXIT_SUCCESS;
}
