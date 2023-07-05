// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkToConstantArrayStrategy_h
#define vtkToConstantArrayStrategy_h

#include "vtkFiltersReductionModule.h" // for export
#include "vtkToImplicitStrategy.h"

VTK_ABI_NAMESPACE_BEGIN
/**
 * @class vtkToConstantArrayStrategy
 *
 * Strategy to be used in conjunction with `vtkToImplicitArrayFilter` to identify and compress
 * constant arrays.
 */
class VTKFILTERSREDUCTION_EXPORT vtkToConstantArrayStrategy final : public vtkToImplicitStrategy
{
public:
  static vtkToConstantArrayStrategy* New();
  vtkTypeMacro(vtkToConstantArrayStrategy, vtkToImplicitStrategy);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Parent API implementing the strategy
   */
  vtkToImplicitStrategy::Optional EstimateReduction(vtkDataArray*) override;
  vtkSmartPointer<vtkDataArray> Reduce(vtkDataArray*) override;
  ///@}

protected:
  vtkToConstantArrayStrategy() = default;
  ~vtkToConstantArrayStrategy() override = default;

private:
  vtkToConstantArrayStrategy(const vtkToConstantArrayStrategy&) = delete;
  void operator=(const vtkToConstantArrayStrategy&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif // vtkToConstantArrayStrategy_h
