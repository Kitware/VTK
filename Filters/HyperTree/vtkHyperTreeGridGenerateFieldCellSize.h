// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridGenerateFieldCellSize
 * @brief vtkHyperTreeGridGenerateFields internal class to define CellSize field
 *
 * This is an internal class used by vtkHyperTreeGridGenerateFields to add and compute the CellSize
 * field.
 *
 * This field is set to the size (volume) of the cell for 3D HTGs,
 * depending on its depth level. This field has a value for every cell traversed through the cursor,
 * valid or not. By extension, CellSize is set to the cell area for 2D HTG and cell length for 1D.
 * In practice, we ignore null size coordinates when computing the value.
 * This field is implemented as an implicit array, in order to lower the memory footprint of the
 * filter.
 */

#ifndef vtkHyperTreeGridGenerateFieldCellSize_h
#define vtkHyperTreeGridGenerateFieldCellSize_h

#include "vtkDoubleArray.h"
#include "vtkHyperTreeGridGenerateField.h"
#include "vtkIndexedArray.h"
#include "vtkUnsignedCharArray.h"

#include <unordered_map>

class vtkHyperTreeGridGenerateFieldCellSize : public vtkHyperTreeGridGenerateField
{
public:
  explicit vtkHyperTreeGridGenerateFieldCellSize(std::string arrayName)
    : vtkHyperTreeGridGenerateField(arrayName)
  {
  }

  void Initialize(vtkHyperTreeGrid* inputHTG) override;
  /**
   * Record the depth of the cell pointed by the cursor in an internal structure.
   * While we have less different size values than an unsigned char can hold, use an index implicit
   * array to save memory. In extreme cases where we cannot (eg. too many levels or custom scales),
   * use a traditional VTK double array.
   * This method does not guarantee thread-safety.
   */
  void Compute(vtkHyperTreeGridNonOrientedGeometryCursor* cursor) override;
  vtkDataArray* GetAndFinalizeArray() override;

private:
  /**
   * Insert size double value into internal storage structures when using indexed arrays.
   * Return true if insertion was successful, and false if the internal structure has one too many
   * values and we should switch to traditional size storage.
   */
  bool InsertSize(double cellSize, vtkIdType currentIndex);
  /**
   * Convert indexed cell values to direct values using a double array.
   * Should be used When switching from indexed implicit array to full-size cell size array.
   */
  void ConvertSizes();

  bool UseIndexedVolume = true;
  std::unordered_map<double, unsigned char> VolumeLookup;
  vtkNew<vtkUnsignedCharArray> SizeIndirectionTable;
  vtkNew<vtkDoubleArray> SizeDiscreteValues;
  vtkNew<vtkDoubleArray> SizeFullValues;

  vtkNew<vtkIndexedArray<double>> OutputSizeArray;
};

#endif // vtkHyperTreeGridGenerateFieldCellSize_h
