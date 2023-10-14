// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkToImplicitArrayFilter_h
#define vtkToImplicitArrayFilter_h

#include "vtkFiltersReductionModule.h" // for module export
#include "vtkPassInputTypeAlgorithm.h"

#include <memory>

VTK_ABI_NAMESPACE_BEGIN

/**
 * @class vtkToImplicitArrayFilter
 *
 * A VTK filter for compressing explicit memory arrays into implicit arrays. The filter operates on
 * arrays using strategies that inherit from `vtkToImplicitStrategy`. Arrays that are not compressed
 * are shallow copied.
 *
 * Here is a code snippet using the filter considering that there is a `previousFilter` with an
 * output data set that has a data array called "Constant" defined on its points:
 * ```
 * vtkNew<vtkToImplicitArrayFilter> toImpArr;
 * vtkNew<vtkToConstantArrayStrategy> strat;
 * toImpArr->SetStrategy(strat);
 * toImpArr->SetInputConnection(previousFilter->GetOutputPort());
 * auto select = toImpArr->GetPointDataArraySelection();
 * select->EnableArray("Constant");
 * toImpArr->Update();
 * ```
 *
 * @sa
 * vtkToImplicitStrategy, vtkImplicitArray
 */

class vtkDataArraySelection;
class vtkToImplicitStrategy;
class VTKFILTERSREDUCTION_EXPORT vtkToImplicitArrayFilter : public vtkPassInputTypeAlgorithm
{
public:
  static vtkToImplicitArrayFilter* New();
  vtkTypeMacro(vtkToImplicitArrayFilter, vtkPassInputTypeAlgorithm);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Setters/Getters for UseMaxNumberOfDegreesOfFreedom
   *
   * Determines whether to use the MaxNumberOfDegreesOfFreedom (true) to accept a reduction or the
   * TargetReduction (false) property (false by default).
   */
  vtkGetMacro(UseMaxNumberOfDegreesOfFreedom, bool);
  vtkSetMacro(UseMaxNumberOfDegreesOfFreedom, bool);
  vtkBooleanMacro(UseMaxNumberOfDegreesOfFreedom, bool);
  ///@}

  ///@{
  /**
   * Setters/Getters for MaxNumberOfDegreesOfFreedom
   *
   * The max number of degrees of freedom to accept in an implicit array for reduction (100 by
   * default).
   *
   * @see SetUseMaxNumberOfDegreesOfFreedom for use case
   */
  vtkGetMacro(MaxNumberOfDegreesOfFreedom, std::size_t);
  vtkSetMacro(MaxNumberOfDegreesOfFreedom, std::size_t);
  ///@}

  ///@{
  /**
   * Setter/Getter for target reduction
   * Value usually in between 0 and 1 which sets the acceptable reduction in size of an array for
   * passing it to its implicit form (0.1 by default).
   */
  vtkGetMacro(TargetReduction, double);
  vtkSetMacro(TargetReduction, double);
  ///@}

  ///@{
  /**
   * Setter/Getter for strategy
   */
  void SetStrategy(vtkToImplicitStrategy*);
  const vtkToImplicitStrategy* GetStrategy() const;
  ///@}

  ///@{
  /**
   * Methods for managing array selections
   */
  vtkDataArraySelection* GetArraySelection(int association);
  vtkDataArraySelection* GetPointDataArraySelection();
  vtkDataArraySelection* GetCellDataArraySelection();
  vtkDataArraySelection* GetFieldDataArraySelection();
  vtkDataArraySelection* GetPointsThenCellsDataArraySelection();
  vtkDataArraySelection* GetVertexDataArraySelection();
  vtkDataArraySelection* GetEdgeDataArraySelection();
  vtkDataArraySelection* GetRowDataArraySelection();
  ///@}

protected:
  vtkToImplicitArrayFilter();
  ~vtkToImplicitArrayFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool UseMaxNumberOfDegreesOfFreedom = false;
  std::size_t MaxNumberOfDegreesOfFreedom = 100;

  double TargetReduction = 0.1;

private:
  vtkToImplicitArrayFilter(const vtkToImplicitArrayFilter&) = delete;
  void operator=(const vtkToImplicitArrayFilter&) = delete;

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};
VTK_ABI_NAMESPACE_END

#endif // vtkToImplicitArrayFilter_h
