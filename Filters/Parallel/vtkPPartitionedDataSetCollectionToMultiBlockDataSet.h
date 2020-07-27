/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPPartitionedDataSetCollectionToMultiBlockDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPPartitionedDataSetCollectionToMultiBlockDataSet
 * @brief parallel version of vtkPartitionedDataSetCollectionToMultiBlockDataSet
 *
 * vtkPPartitionedDataSetCollectionToMultiBlockDataSet is an MPI aware version
 * of the vtkPartitionedDataSetCollectionToMultiBlockDataSet that converts
 * partitioned-dataset-collection to a vtkMultiBlockDataSet.
 *
 * The extra work this filter does it to ensure that each `vtkPartitionedDataSet` instance
 * in the input when replaced by a `vtkMultiPieceDataSet in the output,
 * `vtkMultiPieceDataSet` has piece counts across ranks such the output
 * multiblock structure is identical on all ranks. `vtkPartitionedDataSet` /
 * `vtkPartitionedDataSetCollection` doesn't have this requirement and hence the
 * number of partitions in a `vtkPartitionedDataSet` in the input may not be
 * identical on all ranks. Hence, this extra check is needed.
 */

#ifndef vtkPPartitionedDataSetCollectionToMultiBlockDataSet_h
#define vtkPPartitionedDataSetCollectionToMultiBlockDataSet_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionToMultiBlockDataSet.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPPartitionedDataSetCollectionToMultiBlockDataSet
  : public vtkPartitionedDataSetCollectionToMultiBlockDataSet
{
public:
  static vtkPPartitionedDataSetCollectionToMultiBlockDataSet* New();
  vtkTypeMacro(vtkPPartitionedDataSetCollectionToMultiBlockDataSet,
    vtkPartitionedDataSetCollectionToMultiBlockDataSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the controller to use. By default, initialized to
   * `vtkMultiProcessController::GetGlobalController` in the constructor.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkPPartitionedDataSetCollectionToMultiBlockDataSet();
  ~vtkPPartitionedDataSetCollectionToMultiBlockDataSet() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkMultiProcessController* Controller;

private:
  vtkPPartitionedDataSetCollectionToMultiBlockDataSet(
    const vtkPPartitionedDataSetCollectionToMultiBlockDataSet&) = delete;
  void operator=(const vtkPPartitionedDataSetCollectionToMultiBlockDataSet&) = delete;
};

#endif
