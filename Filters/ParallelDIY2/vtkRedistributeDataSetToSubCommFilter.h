// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkRedistributeDataSetToSubCommFilter
 * @brief redistributes input dataset to specific mpi ranks
 *
 * vtkRedistributeDataSetToSubCommFilter is designed to redistribute data
 * onto the specific processes defined by the vtkProcessGroup passed to
 * the SetSubGroup() method.
 *
 * Internally, this filter first uses vtkRedistributeDataSetFilter to
 * redistribute the data to all processes, then uses vtkDIYAggregateDataSetFilter
 * to aggregate the data onto the target number of processes. In the final
 * step, data is exchanged between processes using the multi-process
 * controller that owns the whole input dataset, specified in SetController().
 *
 * @section vtkRedistributeDataSetToSubCommFilter-SupportedDataTypes  Supported Data Types
 *
 * vtkRedistributeDataSetToSubCommFilter should handle the same data types
 * as vtkRedistributeDataSetFilter, as it uses that filter internally. This
 * includes unstructured grid, as well as multi-block, partitioned data set,
 * and partitioned data set collection.  It can also handle structured data
 * sets, but since vtkRedistributeDataSet is used internally, this filter also
 * results in conversion to unstructured grid.
 */
#ifndef vtkRedistributeDataSetToSubCommFilter_h
#define vtkRedistributeDataSetToSubCommFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersParallelDIY2Module.h" // for export macros
#include "vtkSmartPointer.h"              // for vtkSmartPointer

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN

class vtkMultiProcessController;
class vtkProcessGroup;

class VTKFILTERSPARALLELDIY2_EXPORT vtkRedistributeDataSetToSubCommFilter
  : public vtkDataObjectAlgorithm
{
public:
  static vtkRedistributeDataSetToSubCommFilter* New();
  vtkTypeMacro(vtkRedistributeDataSetToSubCommFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the multi-process controller that owns the whole input dataset.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Set/Get the SubGroup.  The SubGroup is a multi-process controller group
   * that specifies the processes to which the output will be aggregated.  Must
   * be a subset of the Controller.
   */
  void SetSubGroup(vtkProcessGroup*);
  vtkGetObjectMacro(SubGroup, vtkProcessGroup);
  ///@}

  ///@{
  /**
   * Internally, this filter uses vtkRedistributeDataSetFilter to partition
   * the data to all ranks before aggregating it to the desired number of ranks.
   * If it will be run for many time steps over which the geometry does not
   * change, then caching the computed cuts after the first time step can
   * save unnecessary processing.  This behavior is off by default, but can be
   * enabled and queried using these methods.
   */
  void SetEnableCutCaching(bool optimize);
  bool GetEnableCutCaching();
  ///@}

protected:
  vtkRedistributeDataSetToSubCommFilter();
  ~vtkRedistributeDataSetToSubCommFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMultiProcessController* Controller;
  vtkProcessGroup* SubGroup;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internal;
};

VTK_ABI_NAMESPACE_END
#endif
