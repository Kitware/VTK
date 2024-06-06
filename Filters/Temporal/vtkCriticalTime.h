// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkCriticalTime
 * @brief Compute time step at which a threshold value has been reached
 *
 * Given an input that changes over time, the `vtkCriticalTime` filter generates an output
 * with similar structure, with a new data array containing time step values (other arrays are
 * discarded). These values correspond to the time at which a specified threshold criterion has been
 * met for a given point/cell array (at each point/cell respectively). To do so, this filter
 * processes all available time steps. If the criterion is never met for a given point/cell, a NaN
 * value is assigned. The output of this filter is not temporal.
 *
 * The threshold criterion can take three forms:
 * 1) greater than a particular value;
 * 2) less than a particular value;
 * 3) between two values.
 *
 * When the selected array has more than one component, you can use the `SetComponentMode` and
 * `SetSelectedComponent` methods to control which component(s) are considered to check against
 * the threshold criterion:
 * 1) if the component mode is set to `UseSelected`, the `Selected Component` is used. The
 * magnitude can be selected if SelectedComponent is set to the number of components of the array;
 * 2) if the component mode is set to `UseAny`, only one component needs to meet the criterion;
 * 3) if the component mode is set to `UseAll`, all component needs to meet the criterion.
 *
 * The output of this filter correspond to the input with the extra temporal field attached to the
 * points/cells. The name of this array is the same of the selected array with `_critical_time`
 * appended at the end.
 *
 * @note This filters expects that the input topology do not change over time.
 *
 * @note If the key `vtkStreamingDemandDrivenPipeline::NO_PRIOR_TEMPORAL_ACCESS()` is set,
 * typically when running this filter in situ, then the filter runs the time steps one at a time.
 * It requires causing the execution of the filter multiple times externally, by calling
 * `UpdateTimeStep()` in a loop or using another filter that iterates over time downstream,
 * for example. When the key is not set, the filter will execute itself by setting the key
 * `vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING()`. In such case, this filter will produce
 * an array called `time_steps` in the output's `FieldData`. It contains all the time steps that
 * have been processed so far.
 *
 * @sa vtkTemporalAlgorithm vtkTemporalStatistics vtkThreshold
 */

#ifndef vtkCriticalTime_h
#define vtkCriticalTime_h

#include "vtkFiltersTemporalModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkTemporalAlgorithm.h" // For temporal algorithm

#include <limits> // For std::numeric_limits<>::infinity()
#include <memory> // For unique_pointer

#ifndef __VTK_WRAP__
#define vtkPassInputTypeAlgorithm vtkTemporalAlgorithm<vtkPassInputTypeAlgorithm>
#endif

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositeDataSet;
class vtkDataSet;
class vtkFieldData;

class VTKFILTERSTEMPORAL_EXPORT vtkCriticalTime : public vtkPassInputTypeAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  vtkTypeMacro(vtkCriticalTime, vtkPassInputTypeAlgorithm);
#ifndef __VTK_WRAP__
#undef vtkPassInputTypeAlgorithm
#endif
  static vtkCriticalTime* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

#if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML)
  vtkCreateWrappedTemporalAlgorithmInterface();
#endif

  ///@{
  /**
   * Set/get the lower threshold. The default value is set to -infinity.
   */
  vtkGetMacro(LowerThreshold, double);
  vtkSetMacro(LowerThreshold, double);
  ///@}

  ///@{
  /**
   * Set/get the upper threshold. The default value is set to infinity.
   */

  vtkGetMacro(UpperThreshold, double);
  vtkSetMacro(UpperThreshold, double);
  ///@}

  /**
   * Possible values for the threshold criterion:
   * - THRESHOLD_BETWEEN - Values are between the lower and upper thresholds.
   * - THRESHOLD_LOWER - Values are below the lower threshold.
   * - THRESHOLD_UPPER - Values are above the upper threshold.
   */
  enum ThresholdType
  {
    THRESHOLD_BETWEEN = 0,
    THRESHOLD_LOWER,
    THRESHOLD_UPPER
  };

  ///@{
  /**
   * Get/Set the threshold criterion, defining which threshold bounds to use.
   * The default method is `THRESHOLD_UPPER`.
   *
   * Note: values are clamped between THRESHOLD_BETWEEN and THRESHOLD_UPPER.
   */
  vtkSetClampMacro(ThresholdCriterion, int, THRESHOLD_BETWEEN, THRESHOLD_UPPER);
  vtkGetMacro(ThresholdCriterion, int);
  void SetThresholdCriterionToBetween() { this->SetComponentMode(THRESHOLD_BETWEEN); }
  void SetThresholdCriterionToLower() { this->SetComponentMode(THRESHOLD_LOWER); }
  void SetThresholdCriterionToUpper() { this->SetComponentMode(THRESHOLD_UPPER); }
  ///@}

  /**
   * Return a string representation of the threshold criterion
   */
  std::string GetThresholdFunctionAsString() const;

  enum ComponentModeType
  {
    COMPONENT_MODE_USE_SELECTED = 0,
    COMPONENT_MODE_USE_ALL,
    COMPONENT_MODE_USE_ANY
  };

  ///@{
  /**
   * Control how the decision of in / out is made with multi-component data.
   * The choices are to use the selected component (specified in the
   * SelectedComponent ivar), or to look at all components. When looking at
   * all components, the evaluation can pass if all the components satisfy
   * the rule (UseAll) or if any satisfy is (UseAny). The default value is
   * UseSelected.
   */

  vtkSetClampMacro(ComponentMode, int, COMPONENT_MODE_USE_SELECTED, COMPONENT_MODE_USE_ANY);
  vtkGetMacro(ComponentMode, int);
  void SetComponentModeToUseSelected() { this->SetComponentMode(COMPONENT_MODE_USE_SELECTED); }
  void SetComponentModeToUseAll() { this->SetComponentMode(COMPONENT_MODE_USE_ALL); }
  void SetComponentModeToUseAny() { this->SetComponentMode(COMPONENT_MODE_USE_ANY); }
  ///@}

  /**
   * Return a string representation of the component mode
   */
  std::string GetComponentModeAsString() const;

  ///@{
  /**
   * When the component mode is UseSelected, this ivar indicated the selected
   * component. If set to the number of components of the array, threshold
   * will apply on array's magnitude.
   * The default value is 0.
   */
  vtkSetClampMacro(SelectedComponent, int, 0, VTK_INT_MAX);
  vtkGetMacro(SelectedComponent, int);
  ///@}

protected:
  vtkCriticalTime();
  ~vtkCriticalTime() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int Initialize(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int Execute(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int Finalize(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkCriticalTime(const vtkCriticalTime&) = delete;
  void operator=(const vtkCriticalTime&) = delete;

  double LowerThreshold = -std::numeric_limits<double>::infinity();
  double UpperThreshold = std::numeric_limits<double>::infinity();
  int ThresholdCriterion = THRESHOLD_BETWEEN;
  int ComponentMode = COMPONENT_MODE_USE_SELECTED;
  int SelectedComponent = 0;

  struct vtkCriticalTimeInternals;
  std::unique_ptr<vtkCriticalTimeInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif //_vtkCriticalTime_h
