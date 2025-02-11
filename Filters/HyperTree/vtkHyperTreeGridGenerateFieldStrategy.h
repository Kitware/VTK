// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridGenerateFieldStrategy
 * @brief Abstract class for field definition used by vtkHyperTreeGridGenerateFields
 *
 * This is a class used by vtkHyperTreeGridGenerateFields
 * to define the methods that need to be overridden in order to compute new fields for a HTG.
 */

#ifndef vtkHyperTreeGridGenerateFieldStrategy_h
#define vtkHyperTreeGridGenerateFieldStrategy_h

#include "vtkHyperTreeGrid.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkHyperTreeGridGenerateFieldStrategy : public vtkObject
{
public:
  vtkAbstractTypeMacro(vtkHyperTreeGridGenerateFieldStrategy, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
    os << indent << "Array name: " << this->ArrayName << "\n";
  }

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

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridGenerateFieldStrategy_h
