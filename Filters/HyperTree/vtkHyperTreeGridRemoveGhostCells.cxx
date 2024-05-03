// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridRemoveGhostCells.h"
#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridRemoveGhostCells);

namespace
{
/**
 * Recursively process the tree to mask ghost cells. Return true if the current cell has been
 * masked (i.e. already masked in inMask, ghost in inGhost, or all chilren masked and/or ghost).
 */
bool RecursivelyMaskGhost(
  vtkHyperTreeGridNonOrientedCursor* cursor, vtkBitArray* inMask, vtkUnsignedCharArray* inGhost)
{
  vtkIdType currentId = cursor->GetGlobalNodeIndex();
  cursor->SetMask(false);

  if (inMask && inMask->GetValue(currentId))
  {
    cursor->SetMask(true);
    return true;
  }

  if (inGhost->GetTuple1(currentId))
  {
    cursor->SetMask(true);
    return true;
  }

  bool allGhostOrMasked = false;
  if (!cursor->IsLeaf())
  {
    for (int child = 0; child < cursor->GetNumberOfChildren(); ++child)
    {
      cursor->ToChild(child);
      // Coarse cell is masked if it contains only ghosts and/or masked children
      allGhostOrMasked = allGhostOrMasked && RecursivelyMaskGhost(cursor, inMask, inGhost);
      cursor->ToParent();
    }
  }

  cursor->SetMask(allGhostOrMasked);
  return allGhostOrMasked;
}
}

//------------------------------------------------------------------------------
vtkHyperTreeGridRemoveGhostCells::vtkHyperTreeGridRemoveGhostCells()
{
  this->AppropriateOutput = true;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridRemoveGhostCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridRemoveGhostCells::ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to hyper tree grid
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  output->ShallowCopy(input);

  // Retrieve ghost array
  if (!input->HasAnyGhostCells())
  {
    vtkWarningMacro(<< "Input does not have a ghost array. Filter will do nothing.");
    output->SetMask(input->GetMask());
    return 1;
  }

  // Retrieve and copy input mask if it exists
  vtkNew<vtkBitArray> outMask;
  if (input->HasMask())
  {
    outMask->DeepCopy(input->GetMask());
  }
  else
  {
    outMask->SetNumberOfTuples(output->GetNumberOfCells());
  }
  output->SetMask(outMask);

  // Iterate over output HTG and mask ghost cells
  vtkIdType inIndex = 0;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  output->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor;
  while (it.GetNextTree(inIndex))
  {
    if (this->CheckAbort())
    {
      break;
    }

    output->InitializeNonOrientedCursor(outCursor, inIndex, true);
    ::RecursivelyMaskGhost(outCursor, input->GetMask(), input->GetGhostCells());
  }

  // Remove ghost cells array
  output->GetCellData()->RemoveArray(input->GetGhostCells()->GetName());

  return 1;
}
VTK_ABI_NAMESPACE_END
