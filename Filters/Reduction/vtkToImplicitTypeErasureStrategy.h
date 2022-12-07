/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkToImplicitTypeErasureStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkToImplicitTypeErasureStrategy_h
#define vtkToImplicitTypeErasureStrategy_h

#include "vtkFiltersReductionModule.h" // for export
#include "vtkToImplicitStrategy.h"

VTK_ABI_NAMESPACE_BEGIN
/**
 * @class vtkToImplicitTypeErasureStrategy
 *
 * A strategy for compressing arrays by type when applicable (int -> unsigned char for example) and
 * wrapping them behind a `vtkImplicitArray` to furnish the same interface.
 *
 * @sa
 * vtkImplicitArray vtkToImplicitArrayFilter vtkToImplicitStrategy
 */
class VTKFILTERSREDUCTION_EXPORT vtkToImplicitTypeErasureStrategy : public vtkToImplicitStrategy
{
public:
  static vtkToImplicitTypeErasureStrategy* New();
  vtkTypeMacro(vtkToImplicitTypeErasureStrategy, vtkToImplicitStrategy);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Parent API implementing the strategy
   */
  Option<double> EstimateReduction(vtkDataArray*) override;
  vtkSmartPointer<vtkDataArray> Reduce(vtkDataArray*) override;
  ///@}

protected:
  vtkToImplicitTypeErasureStrategy() = default;
  ~vtkToImplicitTypeErasureStrategy() override = default;

private:
  vtkToImplicitTypeErasureStrategy(const vtkToImplicitTypeErasureStrategy&) = delete;
  void operator=(const vtkToImplicitTypeErasureStrategy&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif // vtkToImplicitTypeErasureStrategy_h
