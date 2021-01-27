/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectToPartitionedDataSetCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkDataObjectToPartitionedDataSetCollection
 * @brief convert any dataset to vtkPartitionedDataSetCollection.
 *
 * vtkDataObjectToPartitionedDataSetCollection converts any dataset to a
 * vtkPartitionedDataSetCollection. If the input is a multiblock dataset or an
 * AMR dataset, it creates a vtkDataAssembly for the output
 * vtkPartitionedDataSetCollection that reflects the input's hierarchical
 * organization.
 *
 * @sa vtkDataAssemblyUtilities
 */

#ifndef vtkDataObjectToPartitionedDataSetCollection_h
#define vtkDataObjectToPartitionedDataSetCollection_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkDataObjectToPartitionedDataSetCollection
  : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkDataObjectToPartitionedDataSetCollection* New();
  vtkTypeMacro(
    vtkDataObjectToPartitionedDataSetCollection, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkDataObjectToPartitionedDataSetCollection();
  ~vtkDataObjectToPartitionedDataSetCollection() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkDataObjectToPartitionedDataSetCollection(
    const vtkDataObjectToPartitionedDataSetCollection&) = delete;
  void operator=(const vtkDataObjectToPartitionedDataSetCollection&) = delete;
};

#endif
