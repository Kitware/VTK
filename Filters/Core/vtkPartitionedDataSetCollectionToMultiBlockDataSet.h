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
 * Converts vtkPartitionedDataSetCollection to a vtkMultiBlockDataSet.
 * This simply represents the vtkPartitionedDataSetCollection as a
 * vtkMultiBlockDataSet. Any structure represented in the vtkDataAssembly, for
 * example is lost in this conversion.
 *
 * The vtkPPartitionedDataSetCollectionToMultiBlockDataSet is necessary for
 * distributed cases to ensure that the structure of the output
 * vtkMultiBlockDataSet is consistent on all ranks.
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
