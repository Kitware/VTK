/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConvertToPartitionedDataSetCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkConvertToPartitionedDataSetCollection
 * @brief convert any dataset to vtkPartitionedDataSetCollection.
 *
 * vtkConvertToPartitionedDataSetCollection converts any dataset to a
 * vtkPartitionedDataSetCollection. If the input is a multiblock dataset or an
 * AMR dataset, it creates a vtkDataAssembly for the output
 * vtkPartitionedDataSetCollection that reflects the input's hierarchical
 * organization.
 *
 * @sa vtkDataAssemblyUtilities
 */

#ifndef vtkConvertToPartitionedDataSetCollection_h
#define vtkConvertToPartitionedDataSetCollection_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkConvertToPartitionedDataSetCollection
  : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkConvertToPartitionedDataSetCollection* New();
  vtkTypeMacro(vtkConvertToPartitionedDataSetCollection, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkConvertToPartitionedDataSetCollection();
  ~vtkConvertToPartitionedDataSetCollection() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkConvertToPartitionedDataSetCollection(
    const vtkConvertToPartitionedDataSetCollection&) = delete;
  void operator=(const vtkConvertToPartitionedDataSetCollection&) = delete;
};

#endif
