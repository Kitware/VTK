// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridCellCenterStrategy
 * @brief Define the CellCenter field used in vtkHyperTreeGridGenerateFields
 *
 * This is a class used by vtkHyperTreeGridGenerateFields to add and compute the CellCenter
 * field.
 */

#ifndef vtkHyperTreeGridCellCenterStrategy_h
#define vtkHyperTreeGridCellCenterStrategy_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridGenerateFieldStrategy.h"
#include "vtkNew.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkBitArray;
class vtkDoubleArray;
class vtkUnsignedCharArray;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridCellCenterStrategy
  : public vtkHyperTreeGridGenerateFieldStrategy
{
public:
  static vtkHyperTreeGridCellCenterStrategy* New();
  vtkTypeMacro(vtkHyperTreeGridCellCenterStrategy, vtkHyperTreeGridGenerateFieldStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using vtkHyperTreeGridGenerateFieldStrategy::Initialize;
  /**
   * Init internal variables from `inputHTG`.
   */
  void Initialize(vtkHyperTreeGrid* inputHTG) override;

  using vtkHyperTreeGridGenerateFieldStrategy::Compute;
  /**
   * Compute the center of the cell
   */
  void Compute(vtkHyperTreeGridNonOrientedGeometryCursor* cursor) override;

  /**
   * Return a vtkDoubleArray containing the center of each cell.
   */
  vtkDataArray* GetAndFinalizeArray() override;

private:
  vtkHyperTreeGridCellCenterStrategy();
  ~vtkHyperTreeGridCellCenterStrategy() override;

  // Input data
  vtkUnsignedCharArray* InputGhost = nullptr;

  // Output array
  vtkNew<vtkDoubleArray> CellCentersArray;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridCellCenterStrategy_h
