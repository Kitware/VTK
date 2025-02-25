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
  vtkTypeMacro(vtkHyperTreeGridGenerateFields, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Enable/disable the computation of the CellSize array.
   * Default is true.
   */
  virtual bool GetComputeCellSizeArray() VTK_FUTURE_CONST;
  virtual void SetComputeCellSizeArray(bool enable);
  vtkBooleanMacro(ComputeCellSizeArray, bool);
  ///@}

  ///@{
  /**
   * Get/Set the name used for the cell size array.
   * Defaults to 'CellSize'
   */
  virtual std::string GetCellSizeArrayName() VTK_FUTURE_CONST;
  virtual void SetCellSizeArrayName(std::string name);
  ///@}

  ///@{
  /**
   * Enable/disable the computation of the ValidCell array.
   * Default is true.
   */
  virtual bool GetComputeValidCellArray() VTK_FUTURE_CONST;
  virtual void SetComputeValidCellArray(bool enable);
  vtkBooleanMacro(ComputeValidCellArray, bool);
  ///@}

  ///@{
  /**
   * Get/Set the name used for the cell validity array.
   * Defaults to 'ValidCell'
   */
  virtual std::string GetValidCellArrayName() VTK_FUTURE_CONST;
  virtual void SetValidCellArrayName(std::string name);
  ///@}

  ///@{
  /**
   * Enable/disable the computation of the CellCenter array.
   * Default is true.
   */
  virtual bool GetComputeCellCenterArray() VTK_FUTURE_CONST;
  virtual void SetComputeCellCenterArray(bool enable);
  vtkBooleanMacro(ComputeCellCenterArray, bool);
  ///@}

  ///@{
  /**
   * Get/Set the name used for the cell center array.
   * Defaults to 'CellCenter'
   */
  virtual std::string GetCellCenterArrayName() VTK_FUTURE_CONST;
  virtual void SetCellCenterArrayName(std::string name);
  ///@}

  ///@{
  /**
   * Enable/disable the computation of the TotalVisibleVolume array.
   * Default is true.
   */
  virtual bool GetComputeTotalVisibleVolumeArray() VTK_FUTURE_CONST;
  virtual void SetComputeTotalVisibleVolumeArray(bool enable);
  vtkBooleanMacro(ComputeTotalVisibleVolumeArray, bool);
  ///@}

  ///@{
  /**
   * Get/Set the name used for the total visible volume array.
   * Defaults to 'TotalVisibleVolume'
   */
  virtual std::string GetTotalVisibleVolumeArrayName() VTK_FUTURE_CONST;
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
  std::string DefaultCellCenterArrayName = "CellCenter";

  // Field Data
  std::string DefaultTotalVisibleVolumeArrayName = "TotalVisibleVolume";

  std::unordered_map<std::string, vtkHyperTreeGridGenerateFieldStrategy::Field> Fields;
};

VTK_ABI_NAMESPACE_END
#endif
