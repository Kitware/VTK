// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAppendPartitionedDataSetCollection.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPartitionedDataSetCollectionSource.h"

int TestAppendPartitionedDataSetCollection(int, char*[])
{

  vtkNew<vtkPartitionedDataSetCollectionSource> source1;
  source1->Update();
  auto input1 = source1->GetOutput();
  vtkNew<vtkPartitionedDataSetCollectionSource> source2;
  source2->Update();
  auto input2 = source2->GetOutput();

  vtkNew<vtkAppendPartitionedDataSetCollection> append;
  append->AddInputConnection(source1->GetOutputPort());
  append->AddInputConnection(source2->GetOutputPort());

  append->SetAppendModeToAppendPartitions();
  append->Update();
  auto outputAppended = append->GetOutput();
  if (outputAppended->GetNumberOfPartitionedDataSets() !=
      input1->GetNumberOfPartitionedDataSets() &&
    outputAppended->GetNumberOfPartitionedDataSets() != input2->GetNumberOfPartitionedDataSets())
  {
    return EXIT_FAILURE;
  }
  else
  {
    for (unsigned int i = 0; i < input1->GetNumberOfPartitionedDataSets(); ++i)
    {
      if (outputAppended->GetNumberOfPartitions(i) !=
        input1->GetNumberOfPartitions(i) + input2->GetNumberOfPartitions(i))
      {
        return EXIT_FAILURE;
      }
    }
  }

  append->SetAppendModeToMergePartitions();
  append->Update();
  auto outputMerged = append->GetOutput();
  if (outputMerged->GetNumberOfPartitionedDataSets() != input1->GetNumberOfPartitionedDataSets() &&
    outputMerged->GetNumberOfPartitionedDataSets() != input2->GetNumberOfPartitionedDataSets())
  {
    return EXIT_FAILURE;
  }
  else
  {
    for (unsigned int i = 0; i < input1->GetNumberOfPartitionedDataSets(); ++i)
    {
      if (outputMerged->GetNumberOfPartitions(i) != 1)
      {
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
