// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridRemoveGhostCells
 * @brief   Remove ghost cells and ghost array from the input HTG
 *
 * This filter masks the ghost cells and removes the ghost cell array of the input HTG.
 *
 * @sa vtkHyperTreeGridExtractGhostCells
 */

#ifndef vtkHyperTreeGridRemoveGhostCells_h
#define vtkHyperTreeGridRemoveGhostCells_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkUnsignedCharArray;
class vtkHyperTreeGridNonOrientedCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridRemoveGhostCells : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridRemoveGhostCells* New();
  vtkTypeMacro(vtkHyperTreeGridRemoveGhostCells, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkHyperTreeGridRemoveGhostCells();
  ~vtkHyperTreeGridRemoveGhostCells() override = default;

  /**
   * Main routine to hide or show cells based on their ghost type
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

private:
  vtkHyperTreeGridRemoveGhostCells(const vtkHyperTreeGridRemoveGhostCells&) = delete;
  void operator=(const vtkHyperTreeGridRemoveGhostCells&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridRemoveGhostCells */
