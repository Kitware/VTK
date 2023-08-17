// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkDataAssembly.h>
#include <vtkFDSReader.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkTesting.h>

#include <cstdlib>

namespace
{
template <typename T1, typename T2>
bool testValue(T1 gotVal, T2 expectedVal, const char* valName)
{
  if (gotVal != expectedVal)
  {
    std::cerr << "Wrong " << valName << ". Expected " << expectedVal << ", got " << gotVal
              << std::endl;
    return false;
  }
  return true;
}
}

int TestFDSReader(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }

  std::string dataRoot = testHelper->GetDataRoot();

  // Test RequestInformation
  vtkNew<vtkFDSReader> reader;
  std::string fileName = dataRoot + "/Data/FDSExample/exemple_kitware.smv";
  reader->SetFileName(fileName.c_str());
  reader->UpdateInformation();

  vtkDataAssembly* assembly = reader->GetAssembly();
  if (!testValue(assembly->GetNumberOfChildren(0), 5, "number of root children"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(assembly->GetNumberOfChildren(1), 2, "number of grids"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(assembly->GetNumberOfChildren(2), 4, "number of devices"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(assembly->GetNumberOfChildren(3), 1, "number of hrr"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(assembly->GetNumberOfChildren(4), 10, "number of slices"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(assembly->GetNumberOfChildren(5), 12, "number of boundaries"))
  {
    return EXIT_FAILURE;
  }

  // Test extraction
  reader->AddSelector("/exemple_kitware/Grids/Mesh01");
  reader->AddSelector("/exemple_kitware/Devices/HRR_3D");
  reader->AddSelector("/exemple_kitware/HRR/exemple_kitware_hrr");
  reader->AddSelector("/exemple_kitware/Slices/VelX");
  reader->AddSelector("/exemple_kitware/Boundaries/Mesh01_Blockage_1");
  reader->Update();

  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutput());
  vtkDataAssembly* outAssembly = output->GetDataAssembly();

  if (!testValue(outAssembly->GetNumberOfChildren(0), 5, "number of root children"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(outAssembly->GetNumberOfChildren(1), 1, "number of grids"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(outAssembly->GetNumberOfChildren(2), 1, "number of devices"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(outAssembly->GetNumberOfChildren(3), 1, "number of hrrs"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(outAssembly->GetNumberOfChildren(4), 1, "number of slices"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(outAssembly->GetNumberOfChildren(5), 1, "number of boundaries"))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
