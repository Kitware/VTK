/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPartitionedDataSetCollectionToMultiBlockDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPartitionedDataSetCollectionToMultiBlockDataSet
 * @brief convert vtkPartitionedDataSetCollection to vtkMultiBlockDataSet
 *
 * Converts vtkPartitionedDataSetCollection to a vtkMultiBlockDataSet. If the
 * input vtkPartitionedDataSetCollection has a vtkDataAssembly associated with
 * it, this filter will try to preserve the relationships in the hierarchical
 * representation of the output vtkMultiBlockDataSet. It's not always possible
 * to represent the relationships represented in a vtkDataAssembly as a
 * vtkMultiBlockDataSet. In that case, the output merely represents the
 * structure from the input, and vtkDataAssembly will be disregarded.
 *
 * @sa vtkPPartitionedDataSetCollectionToMultiBlockDataSet
 */

#ifndef vtkPartitionedDataSetCollectionToMultiBlockDataSet_h
#define vtkPartitionedDataSetCollectionToMultiBlockDataSet_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkPartitionedDataSetCollection;

class VTKFILTERSCORE_EXPORT vtkPartitionedDataSetCollectionToMultiBlockDataSet
  : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkPartitionedDataSetCollectionToMultiBlockDataSet* New();
  vtkTypeMacro(vtkPartitionedDataSetCollectionToMultiBlockDataSet, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPartitionedDataSetCollectionToMultiBlockDataSet();
  ~vtkPartitionedDataSetCollectionToMultiBlockDataSet() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  bool Execute(vtkPartitionedDataSetCollection* input, vtkMultiBlockDataSet* output);

private:
  vtkPartitionedDataSetCollectionToMultiBlockDataSet(
    const vtkPartitionedDataSetCollectionToMultiBlockDataSet&) = delete;
  void operator=(const vtkPartitionedDataSetCollectionToMultiBlockDataSet&) = delete;
};

#endif
