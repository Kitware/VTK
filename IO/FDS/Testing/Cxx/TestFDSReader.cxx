/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFDSReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestFDSReader
// .SECTION Description
// TODO

#include <vtkDataAssembly.h>
#include <vtkFDSReader.h>
#include <vtkPartitionedDataSetCollection.h>

#include <cstdlib>

namespace
{
template <typename T1, typename T2>
bool testValue(T1 gotVal, T2 expectedVal, const char* valName)
{
  if (gotVal != expectedVal)
  {
    std::cerr << "Wrong " << valName << ". Expected " << expectedVal << ", got " << gotVal << endl;
    return false;
  }
  return true;
}
}

int TestFDSReader(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Test RequestInformation
  vtkNew<vtkFDSReader> reader;
  reader->SetFileName("/home/tgalland/CSTB/Data/Test.smv"); // TODO : change with real test file
  reader->UpdateInformation();

  vtkDataAssembly* assembly = reader->GetAssembly();
  if (!testValue(assembly->GetNumberOfChildren(0), 4, "number of root children"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(assembly->GetNumberOfChildren(1), 2, "number of devices"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(assembly->GetNumberOfChildren(2), 2, "number of hrr"))
  {
    return EXIT_FAILURE;
  }

  // Test extraction
  reader->AddSelector("/Test/Devices/RUN_devc_1");
  reader->AddSelector("/Test/HRR/RUN_hrr_1");
  reader->Update();

  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutput());
  vtkDataAssembly* outAssembly = output->GetDataAssembly();

  if (!testValue(outAssembly->GetNumberOfChildren(0), 2, "number of root children"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(outAssembly->GetNumberOfChildren(1), 1, "number of devices"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(outAssembly->GetNumberOfChildren(2), 1, "number of hrr"))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
