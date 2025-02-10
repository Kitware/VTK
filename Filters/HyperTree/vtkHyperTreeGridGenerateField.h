// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridGenerateField
 * @brief vtkHyperTreeGridGenerateFields internal abstract class for field definition
 *
 * This is an internal class (that doesn't inherit vtkObject) used by vtkHyperTreeGridGenerateFields
 * to define the methods that need to be overridden in order to compute new fields for a HTG.
 */

#ifndef vtkHyperTreeGridGenerateField_h
#define vtkHyperTreeGridGenerateField_h

#include "vtkHyperTreeGrid.h"

class vtkHyperTreeGridGenerateField
{
public:
  explicit vtkHyperTreeGridGenerateField(std::string arrayName)
    : ArrayName(arrayName)
  {
  }

  virtual ~vtkHyperTreeGridGenerateField() = default;

  /**
   * Initialize internal structures based on the given input HTG.
   */
  virtual void Initialize(vtkHyperTreeGrid* inputHTG) = 0;
  /**
   * Compute the array value for the current cell.
   */
  virtual void Compute(vtkHyperTreeGridNonOrientedGeometryCursor* cursor) = 0;
  /**
   * Build the output size array from internally stored values
   */
  virtual vtkDataArray* GetAndFinalizeArray() = 0;

  ///@{
  /**
   * Get/Set the name of the array containing the data
   */
  std::string GetArrayName() { return this->ArrayName; }
  void SetArrayName(std::string arrayName) { this->ArrayName = arrayName; }
  ///@}

protected:
  std::string ArrayName;
};

#endif // vtkHyperTreeGridGenerateField_h
