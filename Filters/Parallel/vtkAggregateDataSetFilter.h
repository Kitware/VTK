// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAggregateDataSetFilter
 * @brief   Aggregates data sets to a reduced number of processes.
 *
 * This class allows polydata and unstructured grids to be aggregated
 * over a smaller set of processes. The derived vtkDIYAggregateDataSetFilter
 * will operate on image data, rectilinear grids and structured grids.
 */

#ifndef vtkAggregateDataSetFilter_h
#define vtkAggregateDataSetFilter_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkAggregateDataSetFilter : public vtkPassInputTypeAlgorithm
{
public:
  static vtkAggregateDataSetFilter* New();
  vtkTypeMacro(vtkAggregateDataSetFilter, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Number of target processes. Valid values are between 1 and the total
   * number of processes. The default is 1. If a value is passed in that
   * is less than 1 than NumberOfTargetProcesses is changed/kept at 1.
   * If a value is passed in that is greater than the total number of
   * processes then NumberOfTargetProcesses is changed/kept at the
   * total number of processes. This is useful for scripting use cases
   * where later on the script is run with more processes than the
   * current amount.
   */
  void SetNumberOfTargetProcesses(int);
  vtkGetMacro(NumberOfTargetProcesses, int);
  ///@}

  enum TargetProcessType
  {
    ROOT_PROCESS = 0,
    PROCESS_WITH_MOST_POINTS = 1
  };
  ///@{
  /**
   * Get/Set the target process for the aggregate operation.
   *
   * - `ROOT_PROCESS`: data from all processes is aggregated to the root process
   *   of the (sub)controller. This ensures consistent aggregation across timesteps
   *   even when point counts change over time.
   *
   * - `PROCESS_WITH_MOST_POINTS` (default): data from all processes is aggregated
   *   to the process that currently has the most points, minimizing the amount of
   *   data sent over the network. Note that if the process with the most points
   *   changes over time, the aggregation target may vary across timesteps.
   */
  vtkSetClampMacro(TargetProcess, int, ROOT_PROCESS, PROCESS_WITH_MOST_POINTS);
  vtkGetMacro(TargetProcess, int);
  void SetTargetProcessToRootProcess() { this->SetTargetProcess(ROOT_PROCESS); }
  void SetTargetProcessToProcessWithMostPoints()
  {
    this->SetTargetProcess(PROCESS_WITH_MOST_POINTS);
  }
  ///@}

  ///@{
  /**
   * Get/Set if the filter should merge coincidental points
   * Note 1: The filter will only merge points if the ghost cell array doesn't exist
   * Note 2: This option is only taken into account with vtkUnstructuredGrid objects
   * Defaults to On
   */
  vtkSetMacro(MergePoints, bool);
  vtkGetMacro(MergePoints, bool);
  vtkBooleanMacro(MergePoints, bool);
  ///@}

protected:
  vtkAggregateDataSetFilter();
  ~vtkAggregateDataSetFilter() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int NumberOfTargetProcesses;

  bool MergePoints = true;

private:
  vtkAggregateDataSetFilter(const vtkAggregateDataSetFilter&) = delete;
  void operator=(const vtkAggregateDataSetFilter&) = delete;

  int TargetProcess;
};

VTK_ABI_NAMESPACE_END
#endif
