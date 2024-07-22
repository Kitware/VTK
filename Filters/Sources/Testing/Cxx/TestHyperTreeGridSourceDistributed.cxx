// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"

int TestHyperTreeGridSourceDistributed(int argc, char* argv[])
{
  // Initialize MPI Controller
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  constexpr int expectedProcs = 2;
  if (nbRanks != 2)
  {
    vtkErrorWithObjectMacro(nullptr, "Expected " << expectedProcs << " processes, got " << nbRanks);
    controller->Finalize();
    return EXIT_FAILURE;
  }

  std::string threadName = "rank-" + std::to_string(controller->GetLocalProcessId());
  vtkLogger::SetThreadName(threadName);

  // Create HTG Source with process selection
  vtkNew<vtkHyperTreeGridSource> htGrid;
  htGrid->SetMaxDepth(6);
  htGrid->SetDebug(true);
  htGrid->SetDimensions(3, 4, 1);
  htGrid->SetGridScale(1.5, 1., 10.);
  htGrid->SetBranchFactor(2);
  htGrid->SetDescriptor(
    "0RR1RR0R.|.... .R.. RRRR R... R...|.R.. ...R ..RR .R.. R... .... ....|.... "
    "...R ..R. .... .R.. R...|.... .... .R.. ....|....");
  htGrid->UpdatePiece(myRank, nbRanks, 0);
  auto htg = htGrid->GetHyperTreeGridOutput();

  // Test that the right trees appear on selected process
  std::array<std::array<bool, 6>, 2> expectedTreeMasking{
    std::array<bool, 6>{ false, false, true, true, false, false },
    std::array<bool, 6>{ true, true, false, false, true, true },
  };

  vtkIdType inIndex = 0;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  htg->InitializeTreeIterator(it);
  int treeIndex = 0;
  bool success = true;
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  while (it.GetNextTree(inIndex))
  {
    htg->InitializeNonOrientedCursor(cursor, inIndex, true);
    if (expectedTreeMasking[myRank][treeIndex++] != cursor->IsMasked())
    {
      vtkErrorWithObjectMacro(
        nullptr, "Tree #" << treeIndex - 1 << " does not appear on the right process.");
      success = false;
    }
  }

  controller->Finalize();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
