// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkToAffineArrayStrategy_h
#define vtkToAffineArrayStrategy_h

#include "vtkFiltersReductionModule.h" // for export
#include "vtkToImplicitStrategy.h"

VTK_ABI_NAMESPACE_BEGIN
/**
 * @class vtkToAffineArrayStrategy
 *
 * Strategy to transform an explicit array into a `vtkAffineArray`.
 *
 * @sa
 * vtkToImplicitStrategy vtkToImplicitArrayFilter vtkAffineArray
 */
class VTKFILTERSREDUCTION_EXPORT vtkToAffineArrayStrategy final : public vtkToImplicitStrategy
{
public:
  static vtkToAffineArrayStrategy* New();
  vtkTypeMacro(vtkToAffineArrayStrategy, vtkToImplicitStrategy);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Implements parent API
   */
  vtkToImplicitStrategy::Optional EstimateReduction(vtkDataArray*) override;
  vtkSmartPointer<vtkDataArray> Reduce(vtkDataArray*) override;
  ///@}

protected:
  vtkToAffineArrayStrategy() = default;
  ~vtkToAffineArrayStrategy() override = default;

private:
  vtkToAffineArrayStrategy(const vtkToAffineArrayStrategy&) = delete;
  void operator=(const vtkToAffineArrayStrategy&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif // vtkToAffineArrayStrategy_h
