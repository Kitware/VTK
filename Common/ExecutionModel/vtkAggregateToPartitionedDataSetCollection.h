// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkAggregateToPartitionedDataSetCollection
 * @brief Aggregate results in the vtkEndFor
 *
 * vtkAggregateToPartitionedDataSetCollection is an execution aggregator for the
 * `vtkEndFor` filter that insert each iteration result in a partition of a
 * vtkPartitionnedDataSetCollection.
 *
 * @sa vtkEndFor, vtkForEach, vtkExecutionAggregator
 */

#ifndef vtkAggregateToPartitionedDataSetCollection_h
#define vtkAggregateToPartitionedDataSetCollection_h

#include "vtkCommonExecutionModelModule.h" // for export macro
#include "vtkExecutionAggregator.h"

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkAggregateToPartitionedDataSetCollection
  : public vtkExecutionAggregator
{
public:
  static vtkAggregateToPartitionedDataSetCollection* New();
  vtkTypeMacro(vtkAggregateToPartitionedDataSetCollection, vtkExecutionAggregator);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  vtkSmartPointer<vtkDataObject> RequestDataObject(vtkDataObject* input) override;

  /**
   * Push the input dataset at the end of the output vtkPartitionedDataSetCollection.
   */
  bool Aggregate(vtkDataObject* input) override;

  /**
   * Retrieve the constructed vtkPartitionedDataSetCollection
   */
  vtkSmartPointer<vtkDataObject> GetOutputDataObject() override;

  /**
   * Reset the internal vtkPartitionedDataSetCollection
   */
  void Clear() override;

protected:
  vtkAggregateToPartitionedDataSetCollection();
  ~vtkAggregateToPartitionedDataSetCollection() override;

private:
  vtkAggregateToPartitionedDataSetCollection(
    const vtkAggregateToPartitionedDataSetCollection&) = delete;
  void operator=(const vtkAggregateToPartitionedDataSetCollection&) = delete;

  struct Internals;
  std::unique_ptr<Internals> Internal;
};

VTK_ABI_NAMESPACE_END

#endif // vtkAggregateToPartitionedDataSetCollection_h
