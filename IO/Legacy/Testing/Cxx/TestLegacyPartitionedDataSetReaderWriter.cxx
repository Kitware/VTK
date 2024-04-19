// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMappedUnstructuredGridGenerator.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkUnstructuredGrid.h"

int TestLegacyPartitionedDataSetReaderWriter(int, char*[])
{
  vtkUnstructuredGrid *unstructuredGrid1(nullptr), *unstructuredGrid2(nullptr);
  vtkMappedUnstructuredGridGenerator::GenerateUnstructuredGrid(&unstructuredGrid1);
  vtkMappedUnstructuredGridGenerator::GenerateUnstructuredGrid(&unstructuredGrid2);

  // Add field data to check if we keep it in the output of the after writer/reader
  vtkNew<vtkFloatArray> fieldArray;
  fieldArray->SetName("fieldArray");
  fieldArray->SetNumberOfTuples(1);
  fieldArray->SetTuple1(0, 3.14);

  vtkNew<vtkPartitionedDataSet> partitionedDS;
  partitionedDS->SetNumberOfPartitions(4);
  partitionedDS->GetFieldData()->AddArray(fieldArray);

  partitionedDS->SetPartition(0, unstructuredGrid1);
  partitionedDS->SetPartition(3, unstructuredGrid2);
  partitionedDS->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "GRID_1");
  partitionedDS->GetMetaData(3u)->Set(vtkCompositeDataSet::NAME(), "GRID_2");

  int np = partitionedDS->GetNumberOfPartitions();
  vtkLogIf(ERROR, 4 != np, "Expected 4 partitions input data sets, got " << np);
  vtkLogIf(ERROR, nullptr == partitionedDS->GetPartition(0),
    "Expected input data to have data on partition-index 0");
  vtkLogIf(ERROR, nullptr != partitionedDS->GetPartition(1),
    "Expected input data to have no data on partition-index 1");
  vtkLogIf(ERROR, nullptr != partitionedDS->GetPartition(2),
    "Expected input data to have no data on partition-index 2");
  vtkLogIf(ERROR, nullptr == partitionedDS->GetPartition(3),
    "Expected input data to have data on partition-index 3");
  vtkLogIf(ERROR, !partitionedDS->HasMetaData(0u), "Expected metadata on partition index 0");
  vtkLogIf(ERROR, partitionedDS->HasMetaData(1u), "Expected no metadata on partition index 1");
  vtkLogIf(ERROR, partitionedDS->HasMetaData(2u), "Expected no metadata on partition index 2");
  vtkLogIf(ERROR, !partitionedDS->HasMetaData(3u), "Expected metadata on partition index 3");
  vtkLogIf(ERROR, 1 != partitionedDS->GetMetaData(0u)->GetNumberOfKeys(),
    "Expected 1 metadata key n partition index 0");
  vtkLogIf(ERROR, 1 != partitionedDS->GetMetaData(0u)->GetNumberOfKeys(),
    "Expected 1 metadata key n partition index 3");

  vtkNew<vtkGenericDataObjectWriter> writer;
  writer->WriteToOutputStringOn();
  writer->SetInputData(partitionedDS);
  writer->Write();

  auto written = writer->GetOutputString();
  vtkLogIf(ERROR, nullptr == written, "Expected a written string.");

  vtkNew<vtkGenericDataObjectReader> reader;
  reader->ReadFromInputStringOn();
  reader->SetInputString(written);
  reader->Update();
  vtkDataObject* result = reader->GetOutput();
  vtkLogIf(ERROR, nullptr == result, "Expected a non-null result.");

  vtkPartitionedDataSet* readDS = vtkPartitionedDataSet::SafeDownCast(result);
  vtkLogIf(ERROR, !readDS, "Expected non-null result dataset");

  np = readDS->GetNumberOfPartitions();
  vtkLogIf(ERROR, 4 != np, "Expected 4 partitions in result, got " << np);
  vtkLogIf(ERROR, nullptr == readDS->GetPartition(0),
    "Expected result data to have data on partition-index 0");
  vtkLogIf(ERROR, nullptr != readDS->GetPartition(1),
    "Expected result data to have no data on partition-index 1");
  vtkLogIf(ERROR, nullptr != readDS->GetPartition(2),
    "Expected result data to have no data on partition-index 2");
  vtkLogIf(ERROR, nullptr == readDS->GetPartition(3),
    "Expected result data to have data on partition-index 3");
  vtkLogIf(ERROR, !readDS->HasMetaData(0u), "Expected metadata on partition index 0");
  vtkLogIf(ERROR, readDS->HasMetaData(1u), "Expected no metadata on partition index 1");
  vtkLogIf(ERROR, readDS->HasMetaData(2u), "Expected no metadata on partition index 2");
  vtkLogIf(ERROR, !readDS->HasMetaData(3u), "Expected metadata on partition index 3");
  vtkLogIf(ERROR, 1 != readDS->GetMetaData(0u)->GetNumberOfKeys(),
    "Expected 1 metadata key n partition index 0");
  vtkLogIf(ERROR, 1 != readDS->GetMetaData(0u)->GetNumberOfKeys(),
    "Expected 1 metadata key n partition index 3");
  vtkLogIf(ERROR, 1 != readDS->GetFieldData()->HasArray("fieldArray"),
    "Expected result data to have a field data on partition-index 0");

  unstructuredGrid1->Delete();
  unstructuredGrid2->Delete();

  return EXIT_SUCCESS;
}
