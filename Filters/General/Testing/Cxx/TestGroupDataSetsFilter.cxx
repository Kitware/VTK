/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGroupDataSetsFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGroupDataSetsFilter.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
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

  return EXIT_SUCCESS;
}
