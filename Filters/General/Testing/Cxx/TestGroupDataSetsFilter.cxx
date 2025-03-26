// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGroupDataSetsFilter.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSphereSource.h"

int TestGroupDataSetsFilter(int, char*[])
{
  vtkNew<vtkSphereSource> sphere;

  vtkNew<vtkGroupDataSetsFilter> groupie;

  // no inputs, should still work without errors.
  groupie->Update();

  // Add inputs, without names
  groupie->AddInputConnection(sphere->GetOutputPort());
  groupie->AddInputConnection(sphere->GetOutputPort());
  groupie->Update();

  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(groupie->GetOutputDataObject(0));
  vtkLogIfF(ERROR, pdc->GetNumberOfPartitionedDataSets() != 2, "Incorrect number of blocks!");

  // Assign name to only 1.
  groupie->SetInputName(1, "Input1");
  groupie->Update();
  pdc = vtkPartitionedDataSetCollection::SafeDownCast(groupie->GetOutputDataObject(0));
  vtkLogIfF(ERROR, strcmp(pdc->GetMetaData(1)->Get(vtkCompositeDataSet::NAME()), "Input1") != 0,
    "Incorrect name!");

  // change output type.
  groupie->SetOutputTypeToMultiBlockDataSet();
  groupie->Update();
  vtkLogIfF(ERROR, vtkMultiBlockDataSet::SafeDownCast(groupie->GetOutputDataObject(0)) == nullptr,
    "Failed to create vtkMultiBlockDataSet");

  groupie->SetOutputTypeToPartitionedDataSet();
  groupie->ClearInputNames();
  groupie->Update();
  vtkLogIfF(ERROR, vtkPartitionedDataSet::SafeDownCast(groupie->GetOutputDataObject(0)) == nullptr,
    "Failed to create vtkPartitionedDataSet");

  // pass in vtkPartitionedDataSets.
  vtkNew<vtkPartitionedDataSet> pdInput;
  pdInput->SetPartition(0, sphere->GetOutputDataObject(0));
  pdInput->SetPartition(1, sphere->GetOutputDataObject(0));

  groupie->AddInputDataObject(pdInput);
  groupie->Update();
  auto pd = vtkPartitionedDataSet::SafeDownCast(groupie->GetOutputDataObject(0));
  vtkLogIfF(ERROR, pd->GetNumberOfPartitions() != 4, "Incorrect number of partitions.");

  groupie->SetOutputTypeToPartitionedDataSetCollection();
  groupie->Update();
  pdc = vtkPartitionedDataSetCollection::SafeDownCast(groupie->GetOutputDataObject(0));
  vtkLogIfF(ERROR, pdc->GetNumberOfPartitionedDataSets() != 3 || pdc->GetNumberOfPartitions(2) != 2,
    "Incorrect vtkPartitionedDataSetCollection created.");

  // test filter with structured input.
  vtkNew<vtkRTAnalyticSource> rtSource1;
  vtkNew<vtkRTAnalyticSource> rtSource2;
  groupie->RemoveAllInputs();
  groupie->AddInputConnection(rtSource1->GetOutputPort());
  groupie->AddInputConnection(rtSource2->GetOutputPort());
  groupie->Update(); // this will raise errors without the extents fix.

  // Test CombineFirstLayerMultiblock option
  groupie->RemoveAllInputs();
  groupie->SetOutputTypeToMultiBlockDataSet();
  groupie->AddInputConnection(sphere->GetOutputPort());
  groupie->AddInputConnection(sphere->GetOutputPort());

  vtkNew<vtkGroupDataSetsFilter> group2;
  group2->AddInputConnection(groupie->GetOutputPort());
  group2->SetOutputTypeToMultiBlockDataSet();
  group2->Update();

  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::SafeDownCast(group2->GetOutput(0));
  if (!output || output->GetNumberOfBlocks() != 1 ||
    !output->GetBlock(0)->IsA("vtkMultiBlockDataSet"))
  {
    vtkLogF(ERROR, "Output should be a multiblock with a single block, also of type multiblock.");
    return EXIT_FAILURE;
  }

  group2->AddInputConnection(groupie->GetOutputPort());
  group2->CombineFirstLayerMultiblockOn();
  group2->Update();

  output = vtkMultiBlockDataSet::SafeDownCast(group2->GetOutput(0));
  if (!output || output->GetNumberOfBlocks() != 4 || !output->GetBlock(0)->IsA("vtkPolyData"))
  {
    vtkLogF(ERROR, "Output should be a multiblock with 4 polydata blocks");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
