// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridRemoveGhostCells.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLHyperTreeGridReader.h"

#include "vtkLogger.h"

namespace
{
/**
 * Recursively process the Hyper Tree, add ghost cell ids from the ghost array
 * to the ghostIds vector, and add non-ghost cell ids to nonGhostIds
 */
void FillGhostIdsVectors(vtkHyperTreeGridNonOrientedCursor* cursor,
  std::vector<vtkIdType>& ghostIds, std::vector<vtkIdType>& nonGhostIds,
  vtkUnsignedCharArray* ghostArray)
{
  vtkIdType currentId = cursor->GetGlobalNodeIndex();
  if (cursor->IsMasked())
  {
    return;
  }

  if (ghostArray->GetTuple1(currentId))
  {
    ghostIds.emplace_back(currentId);
    return;
  }
  else
  {
    nonGhostIds.emplace_back(currentId);
  }

  if (!cursor->IsLeaf())
  {
    for (int child = 0; child < cursor->GetNumberOfChildren(); ++child)
    {
      cursor->ToChild(child);
      ::FillGhostIdsVectors(cursor, ghostIds, nonGhostIds, ghostArray);
      cursor->ToParent();
    }
  }
}

/**
 * Recursively process the Hyper Tree and add the not hidden cell ids to a vector
 */
void FillUnmaskedIdsVector(
  vtkHyperTreeGridNonOrientedCursor* cursor, std::vector<vtkIdType>& unmaskedIds)
{
  vtkIdType currentId = cursor->GetGlobalNodeIndex();
  if (!cursor->IsMasked())
  {
    unmaskedIds.emplace_back(currentId);
  }

  if (!cursor->IsLeaf() && !cursor->IsMasked())
  {
    for (int child = 0; child < cursor->GetNumberOfChildren(); ++child)
    {
      cursor->ToChild(child);
      ::FillUnmaskedIdsVector(cursor, unmaskedIds);
      cursor->ToParent();
    }
  }
}
}

int TestHyperTreeGridRemoveGhostCells(int argc, char* argv[])
{
  int ret = EXIT_SUCCESS;

  // Read HTG file containing ghost cells
  vtkNew<vtkXMLHyperTreeGridReader> reader;
  char* ghostFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/ghost.htg");
  reader->SetFileName(ghostFile);
  delete[] ghostFile;

  // Remove ghost cells from input HTG
  vtkNew<vtkHyperTreeGridRemoveGhostCells> removeGhosts;
  removeGhosts->SetInputConnection(reader->GetOutputPort());
  removeGhosts->Update();

  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());
  vtkHyperTreeGrid* outputHTG = removeGhosts->GetHyperTreeGridOutput();

  // Go through the input HTG and collect leaf ghost cells and unmasked non-ghost cells.
  std::vector<vtkIdType> inputGhostIds;
  std::vector<vtkIdType> inputNonGhostIds;
  inputGhostIds.reserve(inputHTG->GetNumberOfCells());
  inputNonGhostIds.reserve(inputHTG->GetNumberOfCells());

  vtkUnsignedCharArray* inputGhostArray = inputHTG->GetGhostCells();
  vtkIdType inIndex = 0;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  inputHTG->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  while (it.GetNextTree(inIndex))
  {
    inputHTG->InitializeNonOrientedCursor(cursor, inIndex, true);
    ::FillGhostIdsVectors(cursor, inputGhostIds, inputNonGhostIds, inputGhostArray);
  }

  // Go through the output and collect unmasked cells
  std::vector<vtkIdType> outputUnmasked;
  outputUnmasked.reserve(outputHTG->GetNumberOfCells());
  outputHTG->InitializeTreeIterator(it);
  while (it.GetNextTree(inIndex))
  {
    outputHTG->InitializeNonOrientedCursor(cursor, inIndex, true);
    ::FillUnmaskedIdsVector(cursor, outputUnmasked);
  }

  // All input ghost cells should be masked in the output
  for (vtkIdType ghostId : inputGhostIds)
  {
    if (std::find(outputUnmasked.begin(), outputUnmasked.end(), ghostId) != outputUnmasked.end())
    {
      vtkErrorWithObjectMacro(
        nullptr, << "Ghost cell " << ghostId << " remains unmasked in output HTG but should be.");
      ret = EXIT_FAILURE;
    }
  }

  // All input unmasked non-ghost cells should stay unmasked in the output
  for (vtkIdType nonGhostId : inputNonGhostIds)
  {
    if (std::find(outputUnmasked.begin(), outputUnmasked.end(), nonGhostId) == outputUnmasked.end())
    {
      vtkErrorWithObjectMacro(
        nullptr, << "Cell " << nonGhostId << " has been masked in output HTG but shouldn't.");
      ret = EXIT_FAILURE;
    }
  }

  // The extracted cells don't have a ghost cell array
  if (outputHTG->GetGhostCells() != nullptr)
  {
    vtkErrorWithObjectMacro(
      nullptr, << "Extracted ghost cells should not have a ghost array anymore.");
    ret = EXIT_FAILURE;
  }

  return ret;
}
