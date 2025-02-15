// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMergeTimeFilter
 * @brief   Create a multiblock containing one block per input, with unified timestep list.
 *
 * vtkMergeTimeFilter takes multiple temporal datasets as input and synchronize them.
 *
 * The output data is a multiblock dataset containing one block per input dataset.
 * The output timesteps is the union (or the intersection) of each input timestep lists.
 * Duplicates time values are removed, depending on a tolerance, either absolute or relative.
 *
 * When source time is exactly 0., absolute tolerance is used even in relative mode.
 *
 * Note that the actual merge of timesteps is done during the RequestInformation pass.
 * In the 'Relative' mode, inputs are processed in order and comparison is done with
 * previously processed inputs.
 */

#ifndef vtkMergeTimeFilter_h
#define vtkMergeTimeFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

#include <vector> // Use of dynamically allocated array

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkMergeTimeFilter : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkMergeTimeFilter* New();
  vtkTypeMacro(vtkMergeTimeFilter, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the tolerance for comparing time step
   * values to see if they are close enough to be considered
   * identical. Default is 0.00001
   */
  vtkSetClampMacro(Tolerance, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Set/Get if the tolerance is relative to previous input or absolute.
   *
   * Default is false (absolute tolerance).
   */
  vtkSetMacro(UseRelativeTolerance, bool);
  vtkGetMacro(UseRelativeTolerance, bool);
  vtkBooleanMacro(UseRelativeTolerance, bool);
  ///@}

  ///@{
  /**
   * Set/Get if the merge use intersection instead of union.
   * Default is false (union is used).
   */
  vtkSetMacro(UseIntersection, bool);
  vtkGetMacro(UseIntersection, bool);
  vtkBooleanMacro(UseIntersection, bool);
  ///@}

protected:
  vtkMergeTimeFilter() = default;
  ~vtkMergeTimeFilter() override = default;

  /**
   * Compute global extent and timesteps list, depending on Tolerance.
   */
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Append contribution from each input.
   */
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Reimplemented to create a multiblock from inputs. One block per input.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Override to allow multiple inputs.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Create an ordered combination of given timesteps and current OutputTimeSteps list.
   * Avoid duplicate (use Tolerance).
   */
  void MergeTimeSteps(const std::vector<double>& timesteps);

  /**
   * Look for an input time, either:
   *  - nearly equal to outputTime (@sa SetTolerance, @sa SetUseRelativeTolerance)
   *  - nearest lower than outputTime
   *  - outputTime
   */
  double MapToInputTime(int input, double outputTime);

  /**
   * Return true if t1 and t2 are within absolute or relative Tolerance.
   * When t1 is 0., absolute tolerance is used even in relative mode.
   *
   * @sa SetTolerance, SetUseRelativeTolerance
   */
  bool AreTimesWithinTolerance(double t1, double t2);

  double Tolerance = 0.00001;
  bool UseRelativeTolerance = false;
  bool UseIntersection = false;

  double RequestedTimeValue = 0.;
  std::vector<double> OutputTimeSteps;
  std::vector<std::vector<double>> InputsTimeSteps;

private:
  vtkMergeTimeFilter(const vtkMergeTimeFilter&) = delete;
  void operator=(const vtkMergeTimeFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
