// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridRedistribute.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkStringFormatter.h"
#include "vtkTestUtilities.h"
#include "vtkType.h"
#include "vtkXMLHyperTreeGridReader.h"
#include "vtkXMLMultiBlockDataReader.h"

namespace
{
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
bool CheckTreeDepths(vtkHyperTreeGrid* htg)
{
  vtkDataArray* depthArray = htg->GetCellData()->GetArray("Depth");
  if (!depthArray)
  {
    depthArray = htg->GetCellData()->GetArray("level");
  }

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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
bool TestRedistributeComposite(vtkMPIController* controller)
{
  int myRank = controller->GetLocalProcessId();

  vtkNew<vtkHyperTreeGridSource> source;
  source->SetDescriptor("..RR|........");
  source->SetUseMask(true);
  source->SetMask("0111|11111110");
  source->SetDimensions(3, 3, 1);
  source->SetMaxDepth(2);

  vtkNew<vtkGroupDataSetsFilter> group;
  group->SetInputConnection(source->GetOutputPort());
  group->SetOutputTypeToPartitionedDataSetCollection();

  vtkNew<vtkHyperTreeGridRedistribute> redistribute;
  redistribute->SetInputConnection(group->GetOutputPort());
  redistribute->UpdatePiece(myRank, controller->GetNumberOfProcesses(), 0);
  vtkPartitionedDataSetCollection* outputPDC =
    vtkPartitionedDataSetCollection::SafeDownCast(redistribute->GetOutputDataObject(0));
  vtkPartitionedDataSet* pds =
    vtkPartitionedDataSet::SafeDownCast(outputPDC->GetPartitionedDataSet(0));
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(pds->GetPartitionAsDataObject(0));

  std::array nbTrees{ 2, 1, 1 };
  std::array nbMaskedTrees{ 1, 0, 0 };

  if (!::CheckRedistributeResult(htg, nbTrees, nbMaskedTrees, myRank))
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestRedistributeMultiBlock(vtkMPIController* controller, char* muliblock_name)
{
  int myRank = controller->GetLocalProcessId();

  // Read a .vtm file containing 2 HyperTreeGrid on 3 ranks:
  // First one will be on rank 0, and the second one on rank 1.
  // This way, we make sure that meta information is correctly broadcasted from
  // the only (and changing) non-null rank.

  vtkNew<vtkXMLMultiBlockDataReader> reader;
  reader->SetFileName(muliblock_name);

  vtkNew<vtkHyperTreeGridRedistribute> redistribute;
  redistribute->SetInputConnection(reader->GetOutputPort());
  redistribute->UpdatePiece(myRank, controller->GetNumberOfProcesses(), 0);
  vtkMultiBlockDataSet* outputMB =
    vtkMultiBlockDataSet::SafeDownCast(redistribute->GetOutputDataObject(0));
  vtkHyperTreeGrid* htg1 = vtkHyperTreeGrid::SafeDownCast(outputMB->GetBlock(0));
  vtkHyperTreeGrid* htg2 = vtkHyperTreeGrid::SafeDownCast(outputMB->GetBlock(1));

  std::array nbTrees1{ 17, 17, 16 };
  std::array nbMaskedTrees1{ 0, 0, 0 };

  if (!::CheckRedistributeResult(htg1, nbTrees1, nbMaskedTrees1, myRank))
  {
    return false;
  }

  std::array nbTrees2{ 17, 17, 16 };
  std::array nbMaskedTrees2{ 0, 0, 0 };

  if (!::CheckRedistributeResult(htg2, nbTrees2, nbMaskedTrees2, myRank))
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestRedistributeXML(vtkMPIController* controller, const char* shell_name)
{
  int myRank = controller->GetLocalProcessId();

  vtkNew<vtkXMLHyperTreeGridReader> source;
  source->SetFileName(shell_name);
  source->UpdatePiece(myRank, controller->GetNumberOfProcesses(), 0);

  // Redistribute a HTG read from a file with a single piece.
  // Only rank 0 has valid metadata, we make sure that this data is broadcasted properly to other
  // ranks.
  vtkNew<vtkHyperTreeGridRedistribute> redistribute;
  redistribute->SetInputConnection(source->GetOutputPort());
  redistribute->UpdatePiece(myRank, controller->GetNumberOfProcesses(), 0);
  vtkHyperTreeGrid* outputHTG = redistribute->GetHyperTreeGridOutput();

  std::array nbTrees{ 8, 8, 8 };
  std::array nbMaskedTrees{ 2, 4, 4 };

  if (!::CheckRedistributeResult(outputHTG, nbTrees, nbMaskedTrees, myRank))
  {
    return false;
  }

  return true;
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

  char* shell_name = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/shell_3d.htg");
  char* multiblock_name =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/random_multi_block.vtm");

  std::string threadName = "rank #";
  threadName += vtk::to_string(controller->GetLocalProcessId());
  vtkLogger::SetThreadName(threadName);

  success &= ::TestRedistributeHTG3D(controller);
  success &= ::TestRedistributeHTG2D(controller);
  success &= ::TestRedistributeHTG2DOnOneProcess(controller);
  success &= ::TestRedistributeMultiComponent(controller);
  success &= ::TestRedistributeComposite(controller);
  success &= ::TestRedistributeMultiBlock(controller, multiblock_name);
  success &= ::TestRedistributeXML(controller, shell_name);

  delete[] shell_name;

  controller->Finalize();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
