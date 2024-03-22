// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellAttributeCalculator
 * @brief   Perform a per-cell calculation on a vtkCellAttribute.
 *
 * This empty class serves as a common base class for calculators that
 * compute quantities based on cell-attribute data.
 *
 * Examples of calculators include
 * + computing interpolated values;
 * + computing spatial derivatives (such as the Jacobian or Hessian matrices); or
 * + computing integrals over an entire cell.
 *
 * Each type of calculator provides its own abstract subclass with virtual methods
 * and then per-cell-type, per-attribute-type concrete implementations.
 *
 * @sa vtkCellGridAttribute
 */

#ifndef vtkCellAttributeCalculator_h
#define vtkCellAttributeCalculator_h

#include "vtkCommonDataModelModule.h" // for export
#include "vtkObject.h"
#include "vtkSmartPointer.h" // for API.

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGrid;
class vtkCellMetadata;
class vtkCellAttribute;

class VTKCOMMONDATAMODEL_EXPORT vtkCellAttributeCalculator : public vtkObject
{
public:
  vtkTypeMacro(vtkCellAttributeCalculator, vtkObject);
  static vtkCellAttributeCalculator* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Return an instance of a calculator ready to work on \a cell and \a field.
  template <typename CalculatorType>
  vtkSmartPointer<CalculatorType> Prepare(vtkCellMetadata* cell, vtkCellAttribute* field)
  {
    auto result = this->PrepareForGrid(cell, field);
    return CalculatorType::SafeDownCast(result);
  }

  /// Subclasses should override this to create an instance of their class with member
  /// variables set to perform calculations on the given cell type and field.
  virtual vtkSmartPointer<vtkCellAttributeCalculator> PrepareForGrid(
    vtkCellMetadata* vtkNotUsed(cell), vtkCellAttribute* vtkNotUsed(field))
  {
    return nullptr;
  }

protected:
  vtkCellAttributeCalculator() = default;
  ~vtkCellAttributeCalculator() override = default;

private:
  vtkCellAttributeCalculator(const vtkCellAttributeCalculator&) = delete;
  void operator=(const vtkCellAttributeCalculator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
