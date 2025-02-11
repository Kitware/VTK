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
#include "vtkRearrangeFields.h"

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN

enum class DataArrayType
{
  CELL_DATA = 0,
  FIELD_DATA = 1,
};

class vtkHyperTreeGridGenerateFieldStrategy : public vtkObject
{
public:
  vtkAbstractTypeMacro(vtkHyperTreeGridGenerateFieldStrategy, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
    os << indent << "Array name: " << this->ArrayName << "\n";
    os << indent << "Array type: "
       << (this->ArrayType == DataArrayType::CELL_DATA ? "CELL_DATA" : "FIELD_DATA") << "\n";
  }

  /**
   * Re-implement to initialize internal structures based on the given input HTG.
   */
  virtual void Initialize(vtkHyperTreeGrid* inputHTG) = 0;

  ///@{
  /**
   * Re-implement to compute the data for the current cell.
   * Extra parameters `cellData` and `nameMap` are only used when computing FieldData.
   */
  virtual void Compute(vtkHyperTreeGridNonOrientedGeometryCursor*) {}
  virtual void Compute(vtkHyperTreeGridNonOrientedGeometryCursor*, vtkCellData*,
    std::unordered_map<std::string, std::string>)
  {
  }
  ///@}

  /**
   * Re-implement to build the output array from internally stored values.
   */
  virtual vtkDataArray* GetAndFinalizeArray() = 0;

  ///@{
  /**
   * Get/Set the name of the array containing the data.
   */
  std::string GetArrayName() { return this->ArrayName; }
  void SetArrayName(std::string arrayName) { this->ArrayName = arrayName; }
  ///@}

  ///@{
  /**
   * Get/Set type of the data array.
   */
  DataArrayType GetArrayType() { return this->ArrayType; }
  void SetArrayType(DataArrayType arrayType) { this->ArrayType = arrayType; }
  ///@}

protected:
  std::string ArrayName;
  DataArrayType ArrayType = DataArrayType::CELL_DATA;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridGenerateFieldStrategy_h
