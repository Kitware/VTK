// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridExtractGhostCells
 * @brief   Extract ghost cells from the input HTG and untag them as ghost
 *
 * In practice, the input HTG is shallow copied, and every cell is masked unless it is ghost.
 * Coarse cells are shown if any of their leaves is ghost.
 *
 * The input ghost cell array is renamed and no longer considered as a ghost type array.
 *
 * @sa vtkHyperTreeGridRemoveGhostCells
 */

#ifndef vtkHyperTreeGridExtractGhostCells_h
#define vtkHyperTreeGridExtractGhostCells_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

#include "vtkNew.h" // To instantiate the mask array

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkUnsignedCharArray;
class vtkHyperTreeGridNonOrientedCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridExtractGhostCells
  : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridExtractGhostCells* New();
  vtkTypeMacro(vtkHyperTreeGridExtractGhostCells, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set / Get the name of the ghost cell array in the output.
   */
  vtkSetStringMacro(OutputGhostArrayName);
  vtkGetStringMacro(OutputGhostArrayName);
  ///@}

protected:
  vtkHyperTreeGridExtractGhostCells();
  ~vtkHyperTreeGridExtractGhostCells() override = default;

  /**
   * Main routine to hide or show cells based on their ghost type
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Recursively process the tree to mask non-ghost cells.
   * Return true if at least one leaf if ghost.
   */
  bool RecursivelyMaskNonGhost(vtkHyperTreeGridNonOrientedCursor*);

private:
  char* OutputGhostArrayName = nullptr;
  vtkNew<vtkBitArray> OutMask;
  vtkBitArray* InMask = nullptr;
  vtkUnsignedCharArray* InGhost = nullptr;

  vtkHyperTreeGridExtractGhostCells(const vtkHyperTreeGridExtractGhostCells&) = delete;
  void operator=(const vtkHyperTreeGridExtractGhostCells&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridExtractGhostCells */
