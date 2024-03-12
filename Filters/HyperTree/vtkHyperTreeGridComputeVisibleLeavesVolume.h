// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridComputeVisibleLeavesVolume
 * @brief   Generate a cell field equal to 1 if the cell is a valid leaf and 0 otherwise, as well as
 * a volume field.
 *
 * vtkHyperTreeGridComputeVisibleLeavesVolume creates 2 distinct cell fields.
 * The first one, named 'vtkValidCell', has a value of 1 for leaf (non-refined) cells
 * that are neither masked nor ghost.
 *
 * The second one, named 'vtkVolume',
 * is set to the volume of every invididual cell in the HTG, valid or not.
 *
 * These values can be used for data aggregation or computations over the whole grid.
 * In pratice, the cell field for cell validity is implemented as an implicit array,
 * in order to lower the memory footprint of the filter.
 *
 * Note that the filter needs to be run again if cells are refined after its execution.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was originally written by Jacques-Bernard Lekien, 2023
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridComputeVisibleLeavesVolume_h
#define vtkHyperTreeGridComputeVisibleLeavesVolume_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

#include <memory>
#include <ostream> // for ostream

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
class vtkIndent;
class vtkInformation;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedGeometryCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridComputeVisibleLeavesVolume
  : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridComputeVisibleLeavesVolume* New();
  vtkTypeMacro(vtkHyperTreeGridComputeVisibleLeavesVolume, vtkHyperTreeGridAlgorithm)
  void PrintSelf(ostream& ost, vtkIndent indent) override;

protected:
  vtkHyperTreeGridComputeVisibleLeavesVolume();
  ~vtkHyperTreeGridComputeVisibleLeavesVolume() override = default;

  /**
   * Main routine to extract hyper tree grid levels
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  ///@{
  /**
   * Output type is always HTG (not set by superclass)
   */
  int FillOutputPortInformation(int, vtkInformation*) override;
  ///@}

private:
  vtkHyperTreeGridComputeVisibleLeavesVolume(
    const vtkHyperTreeGridComputeVisibleLeavesVolume&) = delete;
  void operator=(const vtkHyperTreeGridComputeVisibleLeavesVolume&) = delete;

  /**
   * Recursively descend into tree down to leaves
   */
  void ProcessNode(vtkHyperTreeGridNonOrientedGeometryCursor*);

  class vtkInternal;
  std::unique_ptr<vtkInternal> Internal;
};

VTK_ABI_NAMESPACE_END
#endif
