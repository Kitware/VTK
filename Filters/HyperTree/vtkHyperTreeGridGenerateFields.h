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
 * vtkHyperTreeGridCellSizeStrategy vtkHyperTreeGridValidCellStrategy
 * vtkHyperTreeGridGenerateFieldStrategy vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
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
#include "vtkHyperTreeGridGenerateFieldStrategy.h" // For vtkHyperTreeGridGenerateFieldStrategy

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN
class vtkCellData;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedGeometryCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridGenerateFields : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridGenerateFields* New();
  vtkTypeMacro(vtkHyperTreeGridGenerateFields, vtkHyperTreeGridAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) override;

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

  ///@{
  /**
   * Get/Set the name used for the total visible volume array.
   * Defaults to 'TotalVisibleVolume'
   */
  virtual std::string GetTotalVisibleVolumeArrayName();
  virtual void SetTotalVisibleVolumeArrayName(std::string name);
  ///@}

protected:
  vtkHyperTreeGridGenerateFields();
  ~vtkHyperTreeGridGenerateFields() override = default;

  /**
   * Main filter routine : process the HTG cell data and then field data
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

private:
  vtkHyperTreeGridGenerateFields(const vtkHyperTreeGridGenerateFields&) = delete;
  void operator=(const vtkHyperTreeGridGenerateFields&) = delete;

  /**
   * Iterate over the trees and fill output array structures. Output arrays are used as CellData or
   * FieldData depending on `type`.
   */
  void ProcessFields(
    vtkHyperTreeGrid* outputHTG, vtkHyperTreeGrid* input, vtkDataObject::AttributeTypes type);

  /**
   * Process a single tree, recursively descending into the tree, down to leaves
   */
  void ProcessNode(vtkHyperTreeGridNonOrientedGeometryCursor* cursor,
    vtkDataObject::AttributeTypes type, vtkCellData* outputCellData);

  // Cell Data
  std::string DefaultCellSizeArrayName = "CellSize";
  std::string DefaultValidCellArrayName = "ValidCell";

  // Field Data
  std::string DefaultTotalVisibleVolumeArrayName = "TotalVisibleVolume";

  std::unordered_map<std::string, vtkSmartPointer<vtkHyperTreeGridGenerateFieldStrategy>> Fields;
  std::unordered_map<std::string, std::string> FieldsNameMap;
};

VTK_ABI_NAMESPACE_END
#endif
