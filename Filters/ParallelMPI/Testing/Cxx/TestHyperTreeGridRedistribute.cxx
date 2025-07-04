// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridRedistribute.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkType.h"

namespace
{
bool CheckDepthArray(vtkHyperTreeGridNonOrientedCursor* cursor, vtkDataArray* depthArray)
{
  if (!cursor->IsMasked() &&
    depthArray->GetTuple1(cursor->GetGlobalNodeIndex()) != cursor->GetLevel())
  {
    vtkLog(ERROR,
      "Expected depth value " << cursor->IsMasked() << " " << cursor->GetLevel() << " for node "
                              << cursor->GetGlobalNodeIndex() << " but got "
                              << depthArray->GetTuple1(cursor->GetGlobalNodeIndex()));
    return false;
  }

  if (cursor->IsMasked() || cursor->IsLeaf())
  {
    return true;
  }

  bool res = true;
  for (int ichild = 0; ichild < cursor->GetNumberOfChildren(); ++ichild)
  {
    cursor->ToChild(ichild);
    res &= ::CheckDepthArray(cursor, depthArray);
    cursor->ToParent();
  }

  return res;
}

bool CheckTreeDepths(vtkHyperTreeGrid* htg)
{
  vtkDataArray* depthArray = htg->GetCellData()->GetArray("Depth");

  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator inputIterator;
  vtkIdType inTreeIndex = 0;
  htg->InitializeTreeIterator(inputIterator);
  while (inputIterator.GetNextTree(inTreeIndex))
  {
    htg->InitializeNonOrientedCursor(cursor, inTreeIndex);
    if (cursor->HasTree())
    {
      if (!::CheckDepthArray(cursor, depthArray))
      {
        vtkLog(ERROR, "Failed tree " << inTreeIndex);
        return false;
      }
    }
  }

  return true;
}

int CountMaskedTrees(vtkHyperTreeGrid* htg)
{
  int countMaskedTrees = 0;
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator inputIterator;
  htg->InitializeTreeIterator(inputIterator);
  vtkIdType inTreeIndex = 0;
  while (inputIterator.GetNextTree(inTreeIndex))
  {
    htg->InitializeNonOrientedCursor(cursor, inTreeIndex);
    if (cursor->HasTree())
    {
      if (cursor->IsMasked())
      {
        countMaskedTrees++;
      }
    }
  }
  return countMaskedTrees;
}

bool CheckRedistributeResult(vtkHyperTreeGrid* outputHTG, const std::array<int, 3>& nbTrees,
  const std::array<int, 3>& nbMaskedTrees, const int myRank)
{
  if (outputHTG->GetNumberOfNonEmptyTrees() != nbTrees[myRank])
  {
    vtkLog(ERROR, << "Expected " << nbTrees[myRank] << " Trees but got "
                  << outputHTG->GetNumberOfNonEmptyTrees() << " for rank " << myRank);
    return false;
  }

  if (!::CheckTreeDepths(outputHTG))
  {
    vtkLog(ERROR, << "Failed tree depth tests");
    return false;
  }

  // Test masked trees
  int countMaskedTrees = ::CountMaskedTrees(outputHTG);

  if (countMaskedTrees != nbMaskedTrees[myRank])
  {
    vtkLog(ERROR, << "Expected " << nbMaskedTrees[myRank] << " Masked trees but got "
                  << countMaskedTrees << " for rank " << myRank);
    return false;
  }

  return true;
}

bool TestRedistributeHTG3D(vtkMPIController* controller)
{
  int myRank = controller->GetLocalProcessId();

  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetSeed(0);
  source->SetDimensions(6, 6, 3);
  source->SetSplitFraction(0.5);
  source->SetMaskedFraction(0.22);
  source->SetMaxDepth(5);

  vtkNew<vtkHyperTreeGridRedistribute> redistribute;
  redistribute->SetInputConnection(source->GetOutputPort());
  redistribute->UpdatePiece(myRank, controller->GetNumberOfProcesses(), 0);
  vtkHyperTreeGrid* outputHTG = redistribute->GetHyperTreeGridOutput();

  std::array nbTrees{ 17, 17, 16 };
  std::array nbMaskedTrees{ 2, 6, 2 };

  if (!::CheckRedistributeResult(outputHTG, nbTrees, nbMaskedTrees, myRank))
  {
    return false;
  }

  // Redistribute twice and check that it's the same
  vtkNew<vtkHyperTreeGridRedistribute> redistribute2;
  redistribute2->SetInputConnection(redistribute->GetOutputPort());
  redistribute2->UpdatePiece(myRank, controller->GetNumberOfProcesses(), 0);
  vtkHyperTreeGrid* outputHTG2 = redistribute->GetHyperTreeGridOutput();
  return ::CheckRedistributeResult(outputHTG2, nbTrees, nbMaskedTrees, myRank);
}

bool TestRedistributeHTG2D(vtkMPIController* controller)
{
  int myRank = controller->GetLocalProcessId();

  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetSeed(123);
  source->SetDimensions(6, 6, 2);
  source->SetSplitFraction(0.7);
  source->SetMaskedFraction(0.44);
  source->SetMaxDepth(4);

  vtkNew<vtkHyperTreeGridRedistribute> redistribute;
  redistribute->SetInputConnection(source->GetOutputPort());
  redistribute->UpdatePiece(myRank, controller->GetNumberOfProcesses(), 0);
  vtkHyperTreeGrid* outputHTG = redistribute->GetHyperTreeGridOutput();

  std::array nbTrees{ 9, 8, 8 };
  std::array nbMaskedTrees{ 4, 3, 3 };

  return ::CheckRedistributeResult(outputHTG, nbTrees, nbMaskedTrees, myRank);
}

bool TestRedistributeHTG2DOnOneProcess(vtkMPIController* controller)
{
  int myRank = controller->GetLocalProcessId();

  vtkNew<vtkHyperTreeGridSource> source;
  source->SetDescriptor("...R|........");
  source->SetDimensions(3, 3, 2);
  source->SetMaxDepth(2);

  vtkNew<vtkHyperTreeGridRedistribute> redistribute;
  redistribute->SetInputConnection(source->GetOutputPort());
  redistribute->UpdatePiece(myRank, controller->GetNumberOfProcesses(), 0);
  vtkHyperTreeGrid* outputHTG = redistribute->GetHyperTreeGridOutput();

  std::array nbTrees{ 2, 1, 1 };
  std::array nbMaskedTrees{ 0, 0, 0 };

  return ::CheckRedistributeResult(outputHTG, nbTrees, nbMaskedTrees, myRank);
}

bool TestRedistributeMultiComponent(vtkMPIController* controller)
{
  int myRank = controller->GetLocalProcessId();

  vtkNew<vtkHyperTreeGridSource> source;
  source->SetDescriptor("....");
  source->SetDimensions(3, 3, 2);
  source->SetMaxDepth(2);
  source->UpdatePiece(myRank, controller->GetNumberOfProcesses(), myRank);

  vtkHyperTreeGrid* sourceHTG = source->GetHyperTreeGridOutput();
  vtkNew<vtkDoubleArray> velocity;
  velocity->SetNumberOfComponents(3);
  velocity->SetNumberOfTuples(4);
  std::array<std::array<double, 3>, 4> velocityValues{ { { 1.0, 2.0, 3.0 }, { 4.0, 5.0, 6.0 },
    { 7.0, 8.0, 9.0 }, { 10.0, 11.0, 12.0 } } };
  for (vtkIdType treeId = 0; treeId < 4; treeId++)
  {
    velocity->SetTuple3(
      treeId, velocityValues[treeId][0], velocityValues[treeId][1], velocityValues[treeId][2]);
  }
  velocity->SetName("velocity");
  sourceHTG->GetCellData()->AddArray(velocity);

  vtkNew<vtkHyperTreeGridRedistribute> redistribute;
  redistribute->SetInputData(sourceHTG);
  redistribute->UpdatePiece(myRank, controller->GetNumberOfProcesses(), 0);
  vtkHyperTreeGrid* outputHTG = redistribute->GetHyperTreeGridOutput();

  std::array nbTrees{ 2, 1, 1 };
  std::array nbMaskedTrees{ 0, 0, 0 };

  vtkDataArray* outputArray = outputHTG->GetCellData()->GetArray("velocity");
  std::vector<std::vector<vtkIdType>> treeIdsLocal{ { 0, 1 }, { 2 }, { 3 } };

  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator inputIterator;
  outputHTG->InitializeTreeIterator(inputIterator);

  for (size_t id = 0; id < treeIdsLocal[myRank].size(); id++)
  {
    std::array<double, 3> localValue;
    outputArray->GetTuple(id, localValue.data());
    for (size_t comp = 0; comp < 3; comp++)
    {
      if (localValue[comp] != velocityValues[treeIdsLocal[myRank][id]][comp])
      {
        vtkLog(ERROR,
          "Expected component " << comp << " of tree " << treeIdsLocal[myRank][id] << " to be "
                                << velocityValues[treeIdsLocal[myRank][id]][comp] << " but got "
                                << localValue[comp]);
        return false;
      }
    }
  }

  return ::CheckRedistributeResult(outputHTG, nbTrees, nbMaskedTrees, myRank);
}
}

int TestHyperTreeGridRedistribute(int argc, char* argv[])
{
  bool success = true;

  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(controller);

  if (controller->GetNumberOfProcesses() != 3)
  {
    vtkLog(ERROR, << "test run on " << controller->GetNumberOfProcesses()
                  << " ranks (3 expected). Cannot compare result");
    return EXIT_FAILURE;
  }

  std::string threadName = "rank #";
  threadName += std::to_string(controller->GetLocalProcessId());
  vtkLogger::SetThreadName(threadName);

  success &= ::TestRedistributeHTG3D(controller);
  success &= ::TestRedistributeHTG2D(controller);
  success &= ::TestRedistributeHTG2DOnOneProcess(controller);
  success &= ::TestRedistributeMultiComponent(controller);

  controller->Finalize();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
