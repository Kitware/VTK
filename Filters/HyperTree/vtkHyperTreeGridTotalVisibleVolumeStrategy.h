// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridTotalVisibleVolumeStrategy
 * @brief Define the TotalVisibleVolume field data used in vtkHyperTreeGridGenerateFields
 *
 * This is a class used by vtkHyperTreeGridGenerateFields to add and compute the TotalVisibleVolume
 * field data, based on the previously computed cell data ValidCell and CellSize.
 *
 * This field contains the total value of the visible cells volumes. Whether a cell is visible is
 * retrieved from the ValidCell cell data array, and its volume from CellSize.
 */

#ifndef vtkHyperTreeGridTotalVisibleVolumeStrategy_h
#define vtkHyperTreeGridTotalVisibleVolumeStrategy_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridGenerateFieldStrategy.h"
#include "vtkNew.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkCellData;
class vtkDataArray;
class vtkDoubleArray;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedGeometryCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridTotalVisibleVolumeStrategy
  : public vtkHyperTreeGridGenerateFieldStrategy
{
public:
  static vtkHyperTreeGridTotalVisibleVolumeStrategy* New();
  vtkTypeMacro(vtkHyperTreeGridTotalVisibleVolumeStrategy, vtkHyperTreeGridGenerateFieldStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using vtkHyperTreeGridGenerateFieldStrategy::Initialize;
  /**
   * Init internal variables from `inputHTG`.
   */
  bool Initialize(std::unordered_map<std::string, Field> fieldMap) override;

  using vtkHyperTreeGridGenerateFieldStrategy::Compute;
  /**
   * Aggregates the volume of visible cells. Retrieves `ValidCell` and `CellSize` arrays from
   * `cellData` to check if the current cell is visible and get its volume.
   */
  void Compute(vtkHyperTreeGridNonOrientedGeometryCursor* cursor, vtkCellData* cellData,
    std::unordered_map<std::string, Field> fieldMap) override;

  /**
   * Set a unique tuple of `TotalVisibleVolumeArray` to `TotalVisibleVolume` and return the array.
   */
  vtkDataArray* GetAndFinalizeArray() override;

private:
  vtkHyperTreeGridTotalVisibleVolumeStrategy();
  ~vtkHyperTreeGridTotalVisibleVolumeStrategy() override;

  // Aggregated volume
  double TotalVisibleVolume = 0;

  // Output array
  vtkNew<vtkDoubleArray> TotalVisibleVolumeArray;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridTotalVisibleVolumeStrategy_h
