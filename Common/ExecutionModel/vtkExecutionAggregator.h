// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkExecutionAggregator
 * @brief Aggregate the results of the sub-pipeline in vtkEndFor
 *
 * vtkExecutionAggregator is an interface for vtkEndFor aggregators
 * An aggregator is called at the end of each loop and processes the
 * resulting data object.
 * The `GetOutputDataObject` should only be called once the iterations are done,
 * it can be used for data reduction.
 *
 * @sa vtkEndFor, vtkForEach, vtkAggregateToPartitionedDataSetCollection
 */

#ifndef vtkExecutionAggregator_h
#define vtkExecutionAggregator_h

#include "vtkObject.h"

#include "vtkCommonExecutionModelModule.h" // for export macro
#include <vtkDataObject.h>                 // for smart pointer signature
#include <vtkSmartPointer.h>               // for smart pointer signature

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkExecutionAggregator : public vtkObject
{
public:
  vtkTypeMacro(vtkExecutionAggregator, vtkObject);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  /**
   * Create the empty output data set.
   * see vtkAlgorithm.h
   */
  virtual vtkSmartPointer<vtkDataObject> RequestDataObject(vtkDataObject* input);

  /**
   * Called at the end of each iteration with the corresponding data object.
   */
  virtual bool Aggregate(vtkDataObject* input) = 0;

  /**
   * Called after the iterations are done, to retrieve the resulting
   * data object. If a reduction operation is done, it should be implemented here.
   */
  virtual vtkSmartPointer<vtkDataObject> GetOutputDataObject() = 0;

  /**
   * Called after the iterations are done, to clean memory
   * that where used by the Aggregate method.
   * It is called after the output has been retrieved
   */
  virtual void Clear() = 0;

protected:
  vtkExecutionAggregator() = default;
  ~vtkExecutionAggregator() override = default;

private:
  vtkExecutionAggregator(const vtkExecutionAggregator&) = delete;
  void operator=(const vtkExecutionAggregator&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif // vtkExecutionAggregator_h
