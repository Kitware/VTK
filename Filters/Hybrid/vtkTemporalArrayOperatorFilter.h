// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTemporalArrayOperatorFilter
 * @brief   perform simple mathematical operation on a data array at different time
 *
 * This filter computes a simple operation between two time steps of one
 * data array.
 * The mesh of the first time step is used.
 *
 * @sa
 * vtkArrayCalulator
 */

#ifndef vtkTemporalArrayOperatorFilter_h
#define vtkTemporalArrayOperatorFilter_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkMultiTimeStepAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSHYBRID_EXPORT vtkTemporalArrayOperatorFilter : public vtkMultiTimeStepAlgorithm
{
public:
  static vtkTemporalArrayOperatorFilter* New();
  vtkTypeMacro(vtkTemporalArrayOperatorFilter, vtkMultiTimeStepAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum OperatorType
  {
    ADD = 0,
    SUB = 1,
    MUL = 2,
    DIV = 3
  };

  ///@{
  /**
   * @brief Set/Get the operator to apply. Default is ADD (0).
   */
  vtkSetMacro(Operator, int);
  vtkGetMacro(Operator, int);
  ///@}

  ///@{
  /**
   * @brief Set/Get the first time step.
   */
  vtkSetMacro(FirstTimeStepIndex, int);
  vtkGetMacro(FirstTimeStepIndex, int);
  ///@}

  ///@{
  /**
   * @brief Set/Get the second time step.
   */
  vtkSetMacro(SecondTimeStepIndex, int);
  vtkGetMacro(SecondTimeStepIndex, int);
  ///@}

  ///@{
  /**
   * @brief Set/Get the suffix to be append to the output array name.
   * If not specified, output will be suffixed with '_' and the operation
   * type (eg. myarrayname_add).
   */
  vtkSetStringMacro(OutputArrayNameSuffix);
  vtkGetStringMacro(OutputArrayNameSuffix);
  ///@}

  ///@{
  /**
   * Set / Get relative mode.
   * When relative mode is true, this filter operates between the timestep requested
   * by the pipeline and a shifted timestep.
   * When relative mode is false absolute timesteps are used as set by SetFirstTimeStepIndex and
   * SetSecondTimeStepIndex. In that case current pipeline time request is ignored.
   *
   * Default is false.
   *
   * @see SetTimeStepShift
   */
  vtkSetMacro(RelativeMode, bool);
  vtkGetMacro(RelativeMode, bool);
  vtkBooleanMacro(RelativeMode, bool);
  ///}

  ///@{
  /**
   * Set / Get the timestep shift.
   * When RelativeMode is true, TimeStepShift is used to get the second
   * timestep to use, relatively to pipeline time.
   * Default is -1 (uses previous timestep)
   *
   * @see SetRelativeMode
   */
  vtkSetMacro(TimeStepShift, int);
  vtkGetMacro(TimeStepShift, int);
  /// @}

protected:
  vtkTemporalArrayOperatorFilter();
  ~vtkTemporalArrayOperatorFilter() override;

  int FillInputPortInformation(int, vtkInformation*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int Execute(vtkInformation* request, const std::vector<vtkSmartPointer<vtkDataObject>>& inputs,
    vtkInformationVector* outputVector) override;

  int GetInputArrayAssociation();
  virtual vtkDataObject* Process(vtkDataObject*, vtkDataObject*);
  virtual vtkDataObject* ProcessDataObject(vtkDataObject*, vtkDataObject*);
  virtual vtkDataArray* ProcessDataArray(vtkDataArray*, vtkDataArray*);

private:
  vtkTemporalArrayOperatorFilter(const vtkTemporalArrayOperatorFilter&) = delete;
  void operator=(const vtkTemporalArrayOperatorFilter&) = delete;

  /**
   * Return a lower-case string for Operator
   */
  std::string GetOperatorAsString();

  /**
   * Compute first and second timesteps.
   * If RelativeMode is false, simply set First and SecondTimeStepIndex
   * If RelativeMode is true, use input and output information associated to TimeStepShfit.
   */
  void GetTimeStepsToUse(int timeSteps[2]);

  int Operator = OperatorType::ADD;
  int FirstTimeStepIndex = 0;
  int SecondTimeStepIndex = 0;
  int NumberTimeSteps = 0;
  char* OutputArrayNameSuffix = nullptr;

  bool RelativeMode = false;
  int TimeStepShift = -1;
};

VTK_ABI_NAMESPACE_END
#endif
