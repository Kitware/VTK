// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkDGLagrangePoints_h
#define vtkDGLagrangePoints_h

#include "vtkDGCell.h" // For enums.
#include "vtkLagrangePoints.h"

VTK_ABI_NAMESPACE_BEGIN

/**\brief A cell-attribute calculator that will enumerate Lagrange-point.
 *        parameter values for a given basis (I, C, F) and order.
 */
class VTKFILTERSCELLGRID_EXPORT vtkDGLagrangePoints : public vtkLagrangePoints
{
public:
  vtkTypeMacro(vtkDGLagrangePoints, vtkLagrangePoints);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkDGLagrangePoints* New();

  ///@{
  /// Construct an instance of this class for the given \a cell and \a attribute.
  vtkSmartPointer<vtkCellAttributeCalculator> PrepareForGrid(
    vtkCellMetadata* cell, vtkCellAttribute* attribute) override;
  ///@}

  /// Return the parametric coordinate of each degree of freedom.
  bool GetParameters(std::vector<std::vector<double>>& params) const override;

protected:
  vtkDGLagrangePoints() = default;
  ~vtkDGLagrangePoints() override = default;

  vtkDGCell::Shape Shape{ vtkDGCell::Shape::None };
  vtkStringToken FunctionSpace; //!< HGRAD, Bezier, …
  vtkStringToken Basis;         //!< I, C, F, …
  /// The nominal (not total) polynomial order along each parametric axis.
  ///
  /// This is empty for shapes with no parametric axes (i.e., vertices).
  std::vector<int> Order;

private:
  vtkDGLagrangePoints(const vtkDGLagrangePoints&) = delete;
  void operator=(const vtkDGLagrangePoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGLagrangePoints_h
