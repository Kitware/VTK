// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTestUtilities.h"
#include <vtkIOSSReader.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSetCollection.h>

int TestIOSSReadAllFilesToDetermineStructure(int argc, char* argv[])
{
  vtkNew<vtkIOSSReader> reader;
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/FSIobject/FSIobject.inp.EXODUS_DATA.e-s.00001.4.0");
  const std::string fname(fileNameC);
  delete[] fileNameC;
  reader->AddFileName(fname.c_str());
  reader->ReadAllFilesToDetermineStructureOff();
  reader->Update();
  auto output = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));
  if (!output)
  {
    vtkLogF(ERROR, "Expected a vtkPartitionedDataSetCollection but got a %s",
      reader->GetOutputDataObject(0)->GetClassName());
    return EXIT_FAILURE;
  }
  // lets verify that the number of blocks is correct
  if (output->GetNumberOfPartitionedDataSets() != 1)
  {
    vtkLogF(
      ERROR, "Expected 1 partitioned dataset but got %d", output->GetNumberOfPartitionedDataSets());
    return EXIT_FAILURE;
  }
  reader->ReadAllFilesToDetermineStructureOn();
  reader->Update();
  output = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));
  if (!output)
  {
    vtkLogF(ERROR, "Expected a vtkPartitionedDataSetCollection but got a %s",
      reader->GetOutputDataObject(0)->GetClassName());
    return EXIT_FAILURE;
  }
  // lets verify that the number of blocks is correct
  if (output->GetNumberOfPartitionedDataSets() != 4)
  {
    vtkLogF(
      ERROR, "Expected 4 partitioned dataset but got %d", output->GetNumberOfPartitionedDataSets());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
