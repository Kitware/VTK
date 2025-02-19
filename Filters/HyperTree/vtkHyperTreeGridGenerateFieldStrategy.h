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

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGrid.h"          // For vtkHyperTreeGrid
#include "vtkObject.h"

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridGenerateFieldStrategy : public vtkObject
{
public:
  struct Field
  {
    std::string name;
    vtkSmartPointer<vtkHyperTreeGridGenerateFieldStrategy> strategy;
    bool enabled;      // Whether the user asked for this array to be computed.
    bool valid = true; // Whether the array can be computed. Only make sense for Field Data (Cell
                       // Data fields are always valid if enabled).
  };

  vtkHyperTreeGridGenerateFieldStrategy() = default;
  vtkAbstractTypeMacro(vtkHyperTreeGridGenerateFieldStrategy, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
    os << indent << "Array name: " << this->ArrayName << "\n";
    os << indent << "Array type: "
       << (this->ArrayType == vtkDataObject::AttributeTypes::CELL ? "CELL_DATA" : "FIELD_DATA")
       << "\n";
  }

  ///@{
  /**
   * Reimplement to initialize internal structures based on the given input HTG.
   * Only one of these methods should be reimplemented:
   *  - If the strategy creates a cell data array, use `Initialize` with a void return type
   *  - If the strategy creates a field data array, use `Initialize` with a bool return type
   */
  virtual void Initialize(vtkHyperTreeGrid* vtkMaybeUnused(inputHTG)) {}
  virtual bool Initialize(std::unordered_map<std::string, Field>) { return true; }
  ///@}

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
    std::unordered_map<std::string, Field>)
  {
  }
  ///@}

  ///@{
  /**
   * Reimplement to build the output array from internally stored values.
   */
  virtual vtkDataArray* GetAndFinalizeArray() = 0;
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

  ///@{
  /**
   * Get/Set the name of the array containing the data.
   * Default is empty.
   */
  std::string GetArrayName() { return this->ArrayName; }
  void SetArrayName(std::string arrayName) { this->ArrayName = arrayName; }
  ///@}

protected:
  std::string ArrayName;
  vtkDataObject::AttributeTypes ArrayType = vtkDataObject::AttributeTypes::CELL;

private:
  void operator=(const vtkHyperTreeGridGenerateFieldStrategy&) = delete;
  vtkHyperTreeGridGenerateFieldStrategy(const vtkHyperTreeGridGenerateFieldStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridGenerateFieldStrategy_h
