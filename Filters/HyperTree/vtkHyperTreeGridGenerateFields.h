// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridGenerateFields
 * @brief   Generate cell fields for a HTG
 *
 * vtkHyperTreeGridGenerateFields creates 2 distinct (double) cell fields: ValidCell and CellSize
 * See respective internal classes for the content and computation of each field.
 *
 * Note that the filter needs to be run again if cells are refined after its execution.
 *
 * @sa
 * vtkHyperTreeGridGenerateFieldCellSize vtkHyperTreeGridGenerateFieldValidCell
 * vtkHyperTreeGridGenerateField vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was originally written by Jacques-Bernard Lekien, 2023
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridGenerateFields_h
#define vtkHyperTreeGridGenerateFields_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"
#include "vtkHyperTreeGridGenerateField.h"

#include <memory> // unique_ptr
#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
class vtkIndent;
class vtkInformation;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedGeometryCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridGenerateFields : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridGenerateFields* New();
  vtkTypeMacro(vtkHyperTreeGridGenerateFields, vtkHyperTreeGridAlgorithm)
  void PrintSelf(ostream& ost, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the name used for the cell size array.
   * Defaults to 'CellSize'
   */
  virtual std::string GetCellSizeArrayName();
  virtual void SetCellSizeArrayName(std::string name);
  ///@}

  ///@{
  /**
   * Get/Set the name used for the cell validity array.
   * Defaults to 'ValidCell'
   */
  virtual std::string GetValidCellArrayName();
  virtual void SetValidCellArrayName(std::string name);
  ///@}

protected:
  vtkHyperTreeGridGenerateFields();
  ~vtkHyperTreeGridGenerateFields() override = default;

  /**
   * Main filter routine : iterate over the trees and fill output array structures.
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

private:
  vtkHyperTreeGridGenerateFields(const vtkHyperTreeGridGenerateFields&) = delete;
  void operator=(const vtkHyperTreeGridGenerateFields&) = delete;

  /**
   * Process a single tree, recursively descending into the tree, down to leaves
   */
  void ProcessNode(vtkHyperTreeGridNonOrientedGeometryCursor*);

  std::unordered_map<std::string, std::unique_ptr<vtkHyperTreeGridGenerateField>> Fields;
};

VTK_ABI_NAMESPACE_END
#endif
