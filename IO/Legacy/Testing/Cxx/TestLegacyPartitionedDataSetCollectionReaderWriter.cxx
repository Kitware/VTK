// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkGenericDataObjectReader.h>
#include <vtkGenericDataObjectWriter.h>
#include <vtkInformation.h>
#include <vtkLogger.h>
#include <vtkMappedUnstructuredGridGenerator.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkUnstructuredGrid.h>

int TestLegacyPartitionedDataSetCollectionReaderWriter(int, char*[])
{
  vtkUnstructuredGrid *unstructuredGrid1(nullptr), *unstructuredGrid2(nullptr);
  vtkMappedUnstructuredGridGenerator::GenerateUnstructuredGrid(&unstructuredGrid1);
  vtkMappedUnstructuredGridGenerator::GenerateUnstructuredGrid(&unstructuredGrid2);

  vtkNew<vtkPartitionedDataSetCollection> partitionedCollection;
  partitionedCollection->SetNumberOfPartitionedDataSets(2);
  partitionedCollection->SetNumberOfPartitions(0u, 2);
  partitionedCollection->SetNumberOfPartitions(1u, 2);

  partitionedCollection->SetPartition(0u, 0, unstructuredGrid1);
  partitionedCollection->SetPartition(1u, 1, unstructuredGrid2);

  partitionedCollection->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "GRID_1");
  partitionedCollection->GetMetaData(1u)->Set(vtkCompositeDataSet::NAME(), "GRID_2");

  int numberOfDataSets = partitionedCollection->GetNumberOfPartitionedDataSets();
  vtkLogIf(ERROR, 2 != numberOfDataSets,
    "Expected 2 partitioned input data sets, got " << numberOfDataSets);
  vtkLogIf(ERROR, nullptr == partitionedCollection->GetPartition(0u, 0),
    "Expected input data-set 0 to have data on partition-index 0");
  vtkLogIf(ERROR, nullptr != partitionedCollection->GetPartition(0u, 1),
    "Expected input data-set 0 to have no data on partition-index 1");
  vtkLogIf(ERROR, nullptr != partitionedCollection->GetPartition(1u, 0),
    "Expected input data-set 1 to have no data on partition-index 0");
  vtkLogIf(ERROR, nullptr == partitionedCollection->GetPartition(1u, 1),
    "Expected input data-set 1 to have data on partition-index 1");
  vtkLogIf(ERROR, !partitionedCollection->HasMetaData(0u), "Expected metadata on partition 0");
  vtkLogIf(ERROR, !partitionedCollection->HasMetaData(1u), "Expected metadata on partition 1");
  vtkLogIf(ERROR, 1 != partitionedCollection->GetMetaData(0u)->GetNumberOfKeys(),
    "Expected 1 key on the partition 0 metadata");
  vtkLogIf(ERROR, 1 != partitionedCollection->GetMetaData(1u)->GetNumberOfKeys(),
    "Expected 1 key on the partition 1 metadata");

  vtkNew<vtkGenericDataObjectWriter> writer;
  writer->WriteToOutputStringOn();
  writer->SetInputData(partitionedCollection);
  writer->Write();

  auto written = writer->GetOutputString();
  vtkLogIf(ERROR, nullptr == written, "Expected a written string.");

  vtkNew<vtkGenericDataObjectReader> reader;
  reader->ReadFromInputStringOn();
  reader->SetInputString(written);
  reader->Update();
  vtkDataObject* result = reader->GetOutput();
  vtkLogIf(ERROR, nullptr == result, "Expected a non-null result.");

  vtkPartitionedDataSetCollection* readCollection =
    vtkPartitionedDataSetCollection::SafeDownCast(result);
  vtkLogIf(ERROR, !readCollection, "Expected non-null dataset collection");

  numberOfDataSets = readCollection->GetNumberOfPartitionedDataSets();
  vtkLogIf(ERROR, 2 != numberOfDataSets,
    "Expected 2 partitioned result data sets, got " << numberOfDataSets);
  vtkLogIf(ERROR, nullptr == readCollection->GetPartition(0u, 0),
    "Expected result data-set 0 to have data on partition-index 0");
  vtkLogIf(ERROR, nullptr != readCollection->GetPartition(0u, 1),
    "Expected result data-set 0 to have no data on partition-index 1");
  vtkLogIf(ERROR, nullptr != readCollection->GetPartition(1u, 0),
    "Expected result data-set 1 to have no data on partition-index 0");
  vtkLogIf(ERROR, nullptr == readCollection->GetPartition(1u, 1),
    "Expected result data-set 1 to have data on partition-index 1");
  vtkLogIf(ERROR, !readCollection->HasMetaData(0u), "Expected metadata on result partition 0");
  vtkLogIf(ERROR, !readCollection->HasMetaData(1u), "Expected metadata on result partition 1");
  vtkLogIf(ERROR, 1 != readCollection->GetMetaData(0u)->GetNumberOfKeys(),
    "Expected 1 key on the result partition 0 metadata");
  vtkLogIf(ERROR, 1 != readCollection->GetMetaData(1u)->GetNumberOfKeys(),
    "Expected 1 key on the result partition 1 metadata");

  unstructuredGrid1->Delete();
  unstructuredGrid2->Delete();

  return EXIT_SUCCESS;
}
