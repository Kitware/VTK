// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkLagrangePoints_h
#define vtkLagrangePoints_h

#include "vtkCellAttributeCalculator.h"
#include "vtkFiltersCellGridModule.h" // For export macro.

#include <vector>

VTK_ABI_NAMESPACE_BEGIN

/**\brief A cell-attribute calculator that will enumerate Lagrange-point.
 *        parameter values for a given basis (I, C, F) and order.
 */
class VTKFILTERSCELLGRID_EXPORT vtkLagrangePoints : public vtkCellAttributeCalculator
{
public:
  vtkTypeMacro(vtkLagrangePoints, vtkCellAttributeCalculator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /// Subclasses must override this method to produce parameters.
  ///
  /// On success, the \a params vector should contain one entry for the
  /// Lagrange point of each basis function **in the order that basis
  /// functions are reported**. It is important to preserve the ordering
  /// of parameter-space points so they correspond to basis functions
  /// in order for algorithms (such as vtkCellGridChangeBasis) to
  /// function properly.
  virtual bool GetParameters(std::vector<std::vector<double>>& params) const = 0;
  ///@}

protected:
  vtkLagrangePoints() = default;
  ~vtkLagrangePoints() override = default;

private:
  vtkLagrangePoints(const vtkLagrangePoints&) = delete;
  void operator=(const vtkLagrangePoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkLagrangePoints_h
