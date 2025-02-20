// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridCellSizeStrategy
 * @brief  Define the CellSize field used in vtkHyperTreeGridGenerateFields
 *
 * This is a class used by vtkHyperTreeGridGenerateFields to add and compute the CellSize
 * field.
 *
 * This field is set to the size (volume) of the cell for 3D HTGs,
 * depending on its depth level. This field has a value for every cell traversed through the cursor,
 * valid or not. By extension, CellSize is set to the cell area for 2D HTG and cell length for 1D.
 * In practice, we ignore null size coordinates when computing the value.
 * This field is implemented as an implicit array, in order to lower the memory footprint of the
 * filter.
 */

#ifndef vtkHyperTreeGridCellSizeStrategy_h
#define vtkHyperTreeGridCellSizeStrategy_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridGenerateFieldStrategy.h"
#include "vtkIndexedArray.h"
#include "vtkNew.h"

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN

class vtkDoubleArray;
class vtkUnsignedCharArray;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridCellSizeStrategy
  : public vtkHyperTreeGridGenerateFieldStrategy
{
public:
  static vtkHyperTreeGridCellSizeStrategy* New();
  vtkTypeMacro(vtkHyperTreeGridCellSizeStrategy, vtkHyperTreeGridGenerateFieldStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using vtkHyperTreeGridGenerateFieldStrategy::Initialize;
  /**
   * Init internal variables from `inputHTG`.
   */
  void Initialize(vtkHyperTreeGrid* inputHTG) override;

  using vtkHyperTreeGridGenerateFieldStrategy::Compute;
  /**
   * Record the depth of the cell pointed by the cursor in an internal structure.
   * While we have less different size values than an unsigned char can hold, use an index implicit
   * array to save memory. In extreme cases where we cannot (eg. too many levels or custom scales),
   * use a traditional VTK double array.
   * This method does not guarantee thread-safety.
   */
  void Compute(vtkHyperTreeGridNonOrientedGeometryCursor* cursor) override;

  /**
   * If `UseIndexedVolume` is true, build and return the output as an implicit indexed array.
   * Otherwise, return the output as a VTK double array.
   */
  vtkDataArray* GetAndFinalizeArray() override;

private:
  vtkHyperTreeGridCellSizeStrategy();
  ~vtkHyperTreeGridCellSizeStrategy() override;

  /**
   * Insert size double value into internal storage structures when using indexed arrays.
   * Return true if insertion was successful, and false if the internal structure has one too many
   * values and we should switch to traditional size storage.
   */
  bool InsertSize(double cellSize, vtkIdType currentIndex);

  /**
   * Convert indexed cell values to direct values using a double array.
   * Should be used when switching from indexed implicit array to full-size cell size array.
   */
  void ConvertSizes();

  bool UseIndexedVolume = true;
  std::unordered_map<double, unsigned char> VolumeLookup;
  vtkNew<vtkUnsignedCharArray> SizeIndirectionTable;
  vtkNew<vtkDoubleArray> SizeDiscreteValues;
  vtkNew<vtkDoubleArray> SizeFullValues;

  vtkNew<vtkIndexedArray<double>> OutputSizeArray;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridCellSizeStrategy_h
