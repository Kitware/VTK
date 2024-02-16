// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridExtractGhostCells.h"
#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridExtractGhostCells);

//------------------------------------------------------------------------------
vtkHyperTreeGridExtractGhostCells::vtkHyperTreeGridExtractGhostCells()
{
  this->AppropriateOutput = true;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridExtractGhostCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OutputGhostArrayName: "
     << (this->OutputGhostArrayName ? this->OutputGhostArrayName : "(nullptr)") << std::endl;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridExtractGhostCells::ProcessTrees(
  vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to hyper tree grid
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  output->ShallowCopy(input);

  // Retrieve and copy input mask if it exists
  this->InMask = input->HasMask() ? input->GetMask() : nullptr;
  if (this->InMask)
  {
    this->OutMask->DeepCopy(this->InMask);
  }
  else
  {
    this->OutMask->SetNumberOfTuples(output->GetNumberOfCells());
  }
  output->SetMask(this->OutMask);

  // Retrieve ghost array
  if (input->HasAnyGhostCells())
  {
    this->InGhost = input->GetGhostCells();
  }
  else
  {
    vtkWarningMacro(<< "Input does not have a ghost array. Output HTG will be empty.");
  }

  // Iterate over
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

    if (!this->InGhost)
    {
      // Input has no ghost cell, mask the whole tree
      this->OutMask->InsertValue(outCursor->GetGlobalNodeIndex(), 1);
    }
    else
    {
      this->RecursivelyMaskNonGhost(outCursor);
    }
  }

  this->OutMask->Squeeze();

  // Copy the input ghost array and rename it in output
  if (this->InGhost)
  {
    vtkNew<vtkUnsignedCharArray> ghostCopy;
    ghostCopy->ShallowCopy(this->InGhost);
    if (!this->OutputGhostArrayName)
    {
      ghostCopy->SetName("GhostType");
    }
    else
    {
      ghostCopy->SetName(this->OutputGhostArrayName);
    }
    output->GetCellData()->AddArray(ghostCopy);
    output->GetCellData()->RemoveArray(this->InGhost->GetName());
  }

  return 1;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridExtractGhostCells::RecursivelyMaskNonGhost(
  vtkHyperTreeGridNonOrientedCursor* cursor)
{
  vtkIdType currentId = cursor->GetGlobalNodeIndex();
  cursor->SetMask(false);

  if (this->InMask && this->InMask->GetValue(currentId))
  {
    cursor->SetMask(true);
    return false;
  }

  bool hasGhosts = false;
  if (cursor->IsLeaf())
  {
    hasGhosts = this->InGhost->GetTuple1(currentId);
  }
  else
  {
    for (int child = 0; child < cursor->GetNumberOfChildren(); ++child)
    {
      cursor->ToChild(child);
      // Coarse cell is masked if it does not have any ghost leaf
      hasGhosts |= this->RecursivelyMaskNonGhost(cursor);
      cursor->ToParent();
    }
  }

  cursor->SetMask(!hasGhosts);
  return hasGhosts;
}

VTK_ABI_NAMESPACE_END
