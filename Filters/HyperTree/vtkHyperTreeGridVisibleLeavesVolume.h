// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridVisibleLeavesVolume
 * @brief   Generate a cell field equal to 1 if the cell is a valid leaf and 0 otherwise, as well as
 * a volume field.
 *
 * vtkHyperTreeGridVisibleLeavesVolume creates 2 distinct (double) cell fields.
 * The first one, named 'ValidCell' by default, has a value of 1.0 for leaf (non-refined) cells
 * that are neither masked nor ghost, and 0.0 otherwise.
 *
 * The second one, named 'CellVolume' by default, is set to the volume of the cell, depending on its
 * depth level. This field has a value for every cell traversed through the cursor, valid or not.
 *
 * These 2 fields can be used for data aggregation or computations over the whole grid, ie computing
 * the total volume of a given HTG. These fields are implemented as
 * implicit arrays, in order to lower the memory footprint of the filter.
 *
 * Note that the filter needs to be run again if cells are refined after its execution.
 * The volume computation can support at most 256 levels.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was originally written by Jacques-Bernard Lekien, 2023
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridVisibleLeavesVolume_h
#define vtkHyperTreeGridVisibleLeavesVolume_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

#include <memory>  // unique_ptr
#include <ostream> // for ostream

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
class vtkIndent;
class vtkInformation;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedGeometryCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridVisibleLeavesVolume
  : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridVisibleLeavesVolume* New();
  vtkTypeMacro(vtkHyperTreeGridVisibleLeavesVolume, vtkHyperTreeGridAlgorithm)
  void PrintSelf(ostream& ost, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the name used for the cell volume array.
   * Defaults to 'CellVolume'
   */
  vtkGetMacro(CellVolumeArrayName, std::string);
  vtkSetMacro(CellVolumeArrayName, std::string);
  ///@}

  ///@{
  /**
   * Get/Set the name used for the cell validity array.
   * Defaults to 'ValidCell'
   */
  vtkGetMacro(ValidCellArrayName, std::string);
  vtkSetMacro(ValidCellArrayName, std::string);
  ///@}

protected:
  vtkHyperTreeGridVisibleLeavesVolume();
  ~vtkHyperTreeGridVisibleLeavesVolume() override = default;

  /**
   * Main filter routine : iterate over the trees and fill output array structures.
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

private:
  vtkHyperTreeGridVisibleLeavesVolume(const vtkHyperTreeGridVisibleLeavesVolume&) = delete;
  void operator=(const vtkHyperTreeGridVisibleLeavesVolume&) = delete;

  /**
   * Process a single tree, recursively descending into the tree, down to leaves
   */
  void ProcessNode(vtkHyperTreeGridNonOrientedGeometryCursor*);

  class vtkInternal;
  std::unique_ptr<vtkInternal> Internal;

  std::string CellVolumeArrayName = "CellVolume";
  std::string ValidCellArrayName = "ValidCell";
};

VTK_ABI_NAMESPACE_END
#endif
