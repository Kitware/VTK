// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkToImplicitStrategy_h
#define vtkToImplicitStrategy_h

#include "vtkFiltersReductionModule.h" // for export
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
/**
 * @class vtkToImplicitStrategy
 *
 * Pure interface for strategies to transform explicit arrays into implicit arrays. The interface
 * has two main components: an `EstimateReduction` method which estimates by how much this strategy
 * can reduce the memory usage of the array and a `Reduce` method which returns a reduced array.
 */
class vtkDataArray;
class VTKFILTERSREDUCTION_EXPORT vtkToImplicitStrategy : public vtkObject
{
public:
  vtkTypeMacro(vtkToImplicitStrategy, vtkObject);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Getter/Setter for tolerance parameter
   *
   * This tolerance controls how close a compressed value needs to be to the actual value to be
   * considered a match in absolute terms.
   *
   * Default value: 0.001
   */
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);
  ///@}

  /**
   * A helper structure for communicating a result with an optional double value
   */
  struct Optional
  {
    bool IsSome = false;
    double Value;

    Optional()
      : IsSome(false)
    {
    }

    Optional(double val)
      : IsSome(true)
      , Value(val)
    {
    }
  };

  /**
   * Estimate the reduction (if possible) that can be obtained on the array using this strategy
   * - if not possible: the returned Optional.IsSome member will be false
   * - if possible: the returned Option.IsSome member will be true with Optional.Value being the
   * reduction factor
   */
  virtual Optional EstimateReduction(vtkDataArray*) = 0;

  /**
   * Return a reduced version of the input array
   */
  virtual vtkSmartPointer<vtkDataArray> Reduce(vtkDataArray*) = 0;

  /**
   * Destroy any cached variables present in the object (useful for storing calculation results
   * in-between the estimation and reduction phases).
   *
   * The default implementation does nothing.
   */
  virtual void ClearCache() {}

protected:
  vtkToImplicitStrategy() = default;
  ~vtkToImplicitStrategy() override = default;

  double Tolerance = 0.001;

private:
  vtkToImplicitStrategy(const vtkToImplicitStrategy&) = delete;
  void operator=(const vtkToImplicitStrategy&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif // vtkToImplicitStrategy_h
