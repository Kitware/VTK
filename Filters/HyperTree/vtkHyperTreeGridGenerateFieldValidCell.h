// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridGenerateFieldValidCell
 * @brief vtkHyperTreeGridGenerateFields internal class to define ValidCell field
 *
 * This is an internal class used by vtkHyperTreeGridGenerateFields to add and compute the ValidCell
 * field.
 *
 * This field has a value of 1.0 for leaf (non-refined) cells
 * that are neither masked nor ghost, and 0.0 otherwise.
 * This field is implemented as an implicit array, in order to lower the memory footprint of the
 * filter.
 */

#ifndef vtkHyperTreeGridGenerateFieldValidCell_h
#define vtkHyperTreeGridGenerateFieldValidCell_h

#include "vtkHyperTreeGridGenerateField.h"
#include "vtkImplicitArray.h"

/**
 * Implicit array implementation unpacking a bool array to an array of type `ValueType`,
 * reducing the memory footprint of the array by a factor of 8 * 8 if `ValueType` is `double`,
 * while still guaranteeing fast element access using static dispatch.
 */
template <typename ValueType>
struct vtkScalarBooleanImplicitBackend final
{
  /**
   * Build the implicit array using a bit vector to be unpacked.
   *
   * @param values Lookup vector to use
   */
  vtkScalarBooleanImplicitBackend(const std::vector<bool>& values)
    : Values(values)
  {
  }

  /**
   * Templated method called for element access
   *
   * @param _index: Array element id
   * \return Array element in the templated type
   */
  ValueType operator()(const int _index) const { return static_cast<ValueType>(Values[_index]); }

  const std::vector<bool> Values;
};

template <typename T>
using vtkScalarBooleanArray = vtkImplicitArray<vtkScalarBooleanImplicitBackend<T>>;

class vtkHyperTreeGridGenerateFieldValidCell : public vtkHyperTreeGridGenerateField
{
public:
  explicit vtkHyperTreeGridGenerateFieldValidCell(std::string arrayName)
    : vtkHyperTreeGridGenerateField(arrayName)
  {
  }

  void Initialize(vtkHyperTreeGrid* inputHTG) override;
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
  vtkNew<::vtkScalarBooleanArray<double>> ValidCellsImplicitArray;
};

#endif // vtkHyperTreeGridGenerateFieldValidCell_h
