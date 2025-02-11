// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridValidCellStrategy
 * @brief Define the ValidCell field used in vtkHyperTreeGridGenerateFields
 *
 * This is a class used by vtkHyperTreeGridGenerateFields to add and compute the ValidCell
 * field.
 *
 * This field has a value of 1.0 for leaf (non-refined) cells
 * that are neither masked nor ghost, and 0.0 otherwise.
 * This field is implemented as an implicit array, in order to lower the memory footprint of the
 * filter.
 */

#ifndef vtkHyperTreeGridValidCellStrategy_h
#define vtkHyperTreeGridValidCellStrategy_h

#include "vtkHyperTreeGridGenerateFieldStrategy.h"
#include "vtkImplicitArray.h"
#include "vtkScalarBooleanImplicitBackend.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkHyperTreeGridValidCellStrategy : public vtkHyperTreeGridGenerateFieldStrategy
{
public:
  static vtkHyperTreeGridValidCellStrategy* New();
  vtkTypeMacro(vtkHyperTreeGridValidCellStrategy, vtkHyperTreeGridGenerateFieldStrategy)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Init internal variables from `inputHTG`.
   */
  void Initialize(vtkHyperTreeGrid* inputHTG) override;

  using vtkHyperTreeGridGenerateFieldStrategy::Compute;
  /**
   * Compute validity of the current cell.
   * A cell is valid if it is a leaf (non-refined) cell that is neither masked nor ghost.
   */
  void Compute(vtkHyperTreeGridNonOrientedGeometryCursor* cursor) override;

  /**
   * Build valid cell field double array using a vtkScalarBooleanImplicitBackend implicit array
   * unpacking the bit array built before. This cell field has a value of 1.0 for valid (leaf,
   * non-ghost, non-masked) cells, and 0.0 for the others.
   */
  vtkDataArray* GetAndFinalizeArray() override;

private:
  /**
   * Set the valid cell array value to true if the HTG leaf cell `index` is a non-ghost and
   * non-masked cell.
   */
  void SetLeafValidity(const vtkIdType& index);

  // Input data
  vtkBitArray* InputMask = nullptr;
  vtkUnsignedCharArray* InputGhost = nullptr;

  // Operations on bool vector are not atomic. This structure needs to change if this filter is
  // parallelized.
  std::vector<bool> PackedValidCellArray;

  // Output array
  vtkNew<vtkImplicitArray<vtkScalarBooleanImplicitBackend<double>>> ValidCellsImplicitArray;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridValidCellStrategy_h
