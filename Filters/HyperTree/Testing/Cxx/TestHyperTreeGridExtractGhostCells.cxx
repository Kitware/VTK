// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridExtractGhostCells.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLHyperTreeGridReader.h"

#include "vtkLogger.h"

namespace
{
/**
 * Recursively process the Hyper Tree and add ghost cell ids from the ghost array
 * to the outputGhost vector
 */
void FillGhostVector(vtkHyperTreeGridNonOrientedCursor* cursor,
  std::vector<vtkIdType>& outputGhosts, vtkUnsignedCharArray* ghostArray)
{
  vtkIdType currentId = cursor->GetGlobalNodeIndex();
  if (ghostArray->GetTuple1(currentId))
  {
    outputGhosts.emplace_back(currentId);
  }

  if (!cursor->IsLeaf() && !cursor->IsMasked())
  {
    for (int child = 0; child < cursor->GetNumberOfChildren(); ++child)
    {
      cursor->ToChild(child);
      ::FillGhostVector(cursor, outputGhosts, ghostArray);
      cursor->ToParent();
    }
  }
}

/**
 * Recursively process the Hyper Tree and add the not hidden cell ids to a vector
 */
void FillUnmaskedVector(vtkHyperTreeGridNonOrientedCursor* cursor,
  std::vector<vtkIdType>& outputMasked, vtkUnsignedCharArray* ghostArray)
{
  vtkIdType currentId = cursor->GetGlobalNodeIndex();
  if (!cursor->IsMasked())
  {
    outputMasked.emplace_back(currentId);
  }

  if (!cursor->IsLeaf() && !cursor->IsMasked())
  {
    for (int child = 0; child < cursor->GetNumberOfChildren(); ++child)
    {
      cursor->ToChild(child);
      ::FillUnmaskedVector(cursor, outputMasked, ghostArray);
      cursor->ToParent();
    }
  }
}
}

int TestHyperTreeGridExtractGhostCells(int argc, char* argv[])
{
  int ret = EXIT_SUCCESS;

  // Read HTG file containing ghost cells
  vtkNew<vtkXMLHyperTreeGridReader> reader;
  char* ghostFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/ghost.htg");
  reader->SetFileName(ghostFile);
  delete[] ghostFile;

  // Extract ghost cells
  vtkNew<vtkHyperTreeGridExtractGhostCells> extractor;
  extractor->SetOutputGhostArrayName("GhostOut");
  extractor->SetInputConnection(reader->GetOutputPort());
  extractor->Update();
  vtkHyperTreeGrid* extractedGhosts = extractor->GetHyperTreeGridOutput();
  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());

  // Go through the input dataset and collect leaf ghost cells
  std::vector<vtkIdType> outputGhosts;
  outputGhosts.reserve(extractedGhosts->GetNumberOfCells());
  vtkUnsignedCharArray* ghostArray = inputHTG->GetGhostCells();
  vtkIdType inIndex = 0;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  inputHTG->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  while (it.GetNextTree(inIndex))
  {
    inputHTG->InitializeNonOrientedCursor(cursor, inIndex, true);
    ::FillGhostVector(cursor, outputGhosts, ghostArray);
  }

  // Go through the output and collect unmasked leaf cells
  std::vector<vtkIdType> outputUnmasked;
  outputGhosts.reserve(extractedGhosts->GetNumberOfCells());
  extractedGhosts->InitializeTreeIterator(it);
  while (it.GetNextTree(inIndex))
  {
    extractedGhosts->InitializeNonOrientedCursor(cursor, inIndex, true);
    ::FillUnmaskedVector(cursor, outputUnmasked, ghostArray);
  }

  // All ghost cells should be unmasked and vice-versa
  for (vtkIdType ghostId : outputGhosts)
  {
    if (std::find(outputUnmasked.begin(), outputUnmasked.end(), ghostId) == outputUnmasked.end())
    {
      vtkErrorWithObjectMacro(
        nullptr, << "Could not find ghost cell " << ghostId << " in output HTG");
      ret = EXIT_FAILURE;
    }
  }

  for (vtkIdType unmaskedId : outputUnmasked)
  {
    if (std::find(outputGhosts.begin(), outputGhosts.end(), unmaskedId) == outputGhosts.end())
    {
      vtkErrorWithObjectMacro(
        nullptr, << "Could not find unmasked cell " << unmaskedId << " in input HTG ghosts");
      ret = EXIT_FAILURE;
    }
  }

  // The extracted cells don't have a ghost cell array,
  // but the array is kept with another name
  if (extractedGhosts->GetGhostCells() != nullptr)
  {
    vtkErrorWithObjectMacro(
      nullptr, << "Extracted ghost cells should not have a ghost array anymore");
    ret = EXIT_FAILURE;
  }
  if (extractedGhosts->GetCellData()->GetArray("GhostOut") == nullptr)
  {
    vtkErrorWithObjectMacro(nullptr, << "Could not find renamed ghost array in output HTG");
    ret = EXIT_FAILURE;
  }

  return ret;
}
