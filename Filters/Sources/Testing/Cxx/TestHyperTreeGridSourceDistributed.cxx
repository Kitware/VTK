// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"

namespace
{
constexpr int NB_PROCS = 3;

template <int NbTrees>
struct SourceConfig
{
  unsigned int Depth;
  unsigned int BranchFactor;
  std::array<unsigned int, 3> Dimensions;
  std::array<double, 3> GridScale;
  std::string Descriptor;
  std::string Mask;
  std::array<char, NbTrees> ExpectedProcess;
};
}

template <int NbTrees>
bool TestSource(const SourceConfig<NbTrees>& config, int myRank, int nbRanks)
{
  // Create HTG Source with process selection
  vtkNew<vtkHyperTreeGridSource> htGrid;
  htGrid->SetDebug(true);
  htGrid->SetMaxDepth(config.Depth);
  htGrid->SetBranchFactor(config.BranchFactor);
  htGrid->SetDimensions(config.Dimensions.data());
  htGrid->SetGridScale(config.GridScale.data());
  htGrid->SetDescriptor(config.Descriptor.c_str());
  if (!config.Mask.empty())
  {
    htGrid->SetUseMask(true);
    htGrid->SetMask(config.Mask.c_str());
  }

  htGrid->UpdatePiece(myRank, nbRanks, 0);
  auto htg = htGrid->GetHyperTreeGridOutput();

  // Test that the right trees appear on selected process
  vtkIdType inIndex = 0;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  htg->InitializeTreeIterator(it);
  int treeIndex = 0;
  bool success = true;
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  while (it.GetNextTree(inIndex))
  {
    htg->InitializeNonOrientedCursor(cursor, inIndex, true);
    if ((config.ExpectedProcess[treeIndex++] == myRank) == cursor->IsMasked())
    {
      vtkErrorWithObjectMacro(nullptr,
        "Tree #" << treeIndex - 1 << " does not appear on the right process "
                 << "Is masked ");
      success = false;
    }
  }

  return success;
}

int TestHyperTreeGridSourceDistributed(int argc, char* argv[])
{
  // Initialize MPI Controller
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  if (nbRanks != NB_PROCS)
  {
    vtkErrorWithObjectMacro(nullptr, "Expected " << NB_PROCS << " processes, got " << nbRanks);
    controller->Finalize();
    return EXIT_FAILURE;
  }

  std::string threadName = "rank-" + std::to_string(controller->GetLocalProcessId());
  vtkLogger::SetThreadName(threadName);

  SourceConfig<6> source1{ 6, 2, { 3, 4, 1 }, { 1.5, 1., 10. },
    "0RR1RR0R.|.... .R.. RRRR R... R...|.R.. ...R ..RR .R.. R... .... ....|.... "
    "...R ..R. .... .R.. R...|.... .... .R.. ....|....",
    "111111|1111 1111 1111 1111 1111|1111 1111 1111 1111 1111 1111 1111|1111 "
    "1111 1111 1111 1111 1111|1111 1111 1111 1111|1111",
    { 0, 0, 1, 1, 0, 0 } };

  SourceConfig<4> source2{
    1,
    1,
    { 3, 3, 1 },
    { 1.5, 1., 10. },
    "0..2.1.",
    "1011",
    { 0, -1, 2, 1 },
  };

  bool success = true;
  success &= ::TestSource(source1, myRank, nbRanks);
  success &= ::TestSource(source2, myRank, nbRanks);

  // Default to 0, ignore chars at the end
  source2.Descriptor = "...2.101";
  source2.Mask = "1011";
  source2.ExpectedProcess = { 0, -1, 0, 2 };
  success &= ::TestSource(source2, myRank, nbRanks);

  source2.Descriptor = ".1.0.2.";
  source2.Mask = "1011";
  source2.ExpectedProcess = { 0, -1, 0, 2 };
  success &= ::TestSource(source2, myRank, nbRanks);

  controller->Finalize();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
