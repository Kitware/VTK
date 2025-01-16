// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkEndFor
 * @brief vtkEndFor define the end of the sub-pipeline to loop
 *
 * vtkEndFor works together with vtkForEach. It marks the end of the loop.
 * Its goals is to use the given `vtkExecutionAggregator` to process the result
 * of each iteration and provide an output dataset.
 *
 * The default aggregator is vtkAggregateToPartitionedDataSetCollection, which
 * build a vtkPartitionedDataSetCollection with each result in a separate partition.
 *
 * > Largely inspired by the ttkForEach/ttkEndFor in the TTK project
 * > (https://github.com/topology-tool-kit/ttk/tree/dev)
 *
 * @sa vtkForEach, vtkExecutionAggregator
 */

#ifndef vtkEndFor_h
#define vtkEndFor_h

#include "vtkCommonExecutionModelModule.h" // for export macro
#include "vtkDataObjectAlgorithm.h"

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN

class vtkExecutionAggregator;
class VTKCOMMONEXECUTIONMODEL_EXPORT vtkEndFor : public vtkDataObjectAlgorithm
{
public:
  static vtkEndFor* New();
  vtkTypeMacro(vtkEndFor, vtkDataObjectAlgorithm);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  /**
   * Aggregator object to use to reduce / aggregate results of for loop
   */
  virtual void SetAggregator(vtkExecutionAggregator*);

protected:
  vtkEndFor();
  ~vtkEndFor() override;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkEndFor(const vtkEndFor&) = delete;
  void operator=(const vtkEndFor&) = delete;

  struct Internals;
  std::unique_ptr<Internals> Internal;
};

VTK_ABI_NAMESPACE_END

#endif // vtkEndFor_h
