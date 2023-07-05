// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConvertToPartitionedDataSetCollection.h"
#include "vtkDataAssembly.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

int TestDataObjectToPartitionedDataSetCollection(int, char*[])
{
  vtkNew<vtkMultiBlockDataSet> mb;
  mb->SetNumberOfBlocks(2);

  vtkNew<vtkMultiBlockDataSet> b0;
  b0->SetNumberOfBlocks(2);
  b0->SetBlock(0, vtkNew<vtkPolyData>());
  b0->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "Block-0/0");
  b0->GetMetaData(1)->Set(vtkCompositeDataSet::NAME(), "Block-0/1");

  mb->SetBlock(0, b0);
  mb->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "Block-0");

  mb->SetBlock(1, vtkNew<vtkUnstructuredGrid>());
  mb->GetMetaData(1)->Set(vtkCompositeDataSet::NAME(), "Block-1");

  vtkNew<vtkConvertToPartitionedDataSetCollection> convertor;
  convertor->SetInputDataObject(mb);
  convertor->Update();

  auto output = vtkPartitionedDataSetCollection::SafeDownCast(convertor->GetOutputDataObject(0));
  if (output == nullptr || output->GetNumberOfPartitionedDataSets() != 3 ||
    vtkPolyData::SafeDownCast(output->GetPartitionedDataSet(0)->GetPartition(0)) == nullptr ||
    output->GetPartitionedDataSet(1)->GetNumberOfPartitions() != 0 ||
    vtkUnstructuredGrid::SafeDownCast(output->GetPartitionedDataSet(2)->GetPartition(0)) == nullptr)
  {
    vtkLogF(ERROR, "Failed to convert multiblock!");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
