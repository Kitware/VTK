// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridValidCellStrategy
 * @brief Define the ValidCell field used in vtkHyperTreeGridGenerateFields
 *
 * This is a class used by vtkHyperTreeGridGenerateFields to add and compute the ValidCell
 * field.
 *
 * This field has a value of 1 for leaf (non-refined) cells
 * that are neither masked nor ghost, and 0 otherwise.
 */

#ifndef vtkHyperTreeGridValidCellStrategy_h
#define vtkHyperTreeGridValidCellStrategy_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridGenerateFieldStrategy.h"
#include "vtkNew.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkBitArray;
class vtkUnsignedCharArray;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridValidCellStrategy
  : public vtkHyperTreeGridGenerateFieldStrategy
{
public:
  static vtkHyperTreeGridValidCellStrategy* New();
  vtkTypeMacro(vtkHyperTreeGridValidCellStrategy, vtkHyperTreeGridGenerateFieldStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using vtkHyperTreeGridGenerateFieldStrategy::Initialize;
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
   * Return a vtkBitArray containing the validity of each cell.
   */
  vtkDataArray* GetAndFinalizeArray() override;

private:
  vtkHyperTreeGridValidCellStrategy();
  ~vtkHyperTreeGridValidCellStrategy() override;

  // Input data
  vtkUnsignedCharArray* InputGhost = nullptr;

  // Output array
  vtkNew<vtkBitArray> ValidCellsArray;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridValidCellStrategy_h
