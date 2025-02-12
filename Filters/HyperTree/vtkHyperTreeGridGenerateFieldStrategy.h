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
#include "vtkObject.h"

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN

class vtkHyperTreeGridGenerateFieldStrategy : public vtkObject
{
public:
  vtkAbstractTypeMacro(vtkHyperTreeGridGenerateFieldStrategy, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
    os << indent << "Array name: " << this->ArrayName << "\n";
    os << indent << "Array type: "
       << (this->ArrayType == vtkDataObject::AttributeTypes::CELL ? "CELL_DATA" : "FIELD_DATA")
       << "\n";
  }

  /**
   * Reimplement to initialize internal structures based on the given input HTG.
   */
  virtual void Initialize(vtkHyperTreeGrid* inputHTG) = 0;

  ///@{
  /**
   * Reimplement to compute the data for the current cell.
   * Only one of these methods should be reimplemented:
   *  - If the strategy creates a cell data array, use `Compute` with a single parameter
   *  - If the strategy creates a field data array, use `Compute` with the extra parameters
   *  (they provide the result of the previously computed cell data).
   */
  virtual void Compute(vtkHyperTreeGridNonOrientedGeometryCursor*) {}
  virtual void Compute(vtkHyperTreeGridNonOrientedGeometryCursor*, vtkCellData*,
    std::unordered_map<std::string, std::string>)
  {
  }
  ///@}

  /**
   * Reimplement to build the output array from internally stored values.
   */
  virtual vtkDataArray* GetAndFinalizeArray() = 0;

  ///@{
  /**
   * Get/Set the name of the array containing the data.
   * Default is empty.
   */
  std::string GetArrayName() { return this->ArrayName; }
  void SetArrayName(std::string arrayName) { this->ArrayName = arrayName; }
  ///@}

  ///@{
  /**
   * Get/Set type of the data array.
   * Only CELL and FIELD are supported for now.
   * Default is CELL.
   */
  vtkDataObject::AttributeTypes GetArrayType() { return this->ArrayType; }
  void SetArrayType(vtkDataObject::AttributeTypes arrayType)
  {
    assert(arrayType == vtkDataObject::AttributeTypes::CELL ||
      arrayType == vtkDataObject::AttributeTypes::FIELD);
    this->ArrayType = arrayType;
  }
  ///@}

protected:
  std::string ArrayName;
  vtkDataObject::AttributeTypes ArrayType = vtkDataObject::AttributeTypes::CELL;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridGenerateFieldStrategy_h
