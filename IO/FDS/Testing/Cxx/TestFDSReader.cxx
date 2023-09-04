// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkDataAssembly.h>
#include <vtkFDSReader.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRectilinearGrid.h>
#include <vtkTable.h>
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
  reader->SetFileName(fileName);
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

  // Test Mesh01
  auto nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("Mesh01"));
  auto mesh01 =
    vtkRectilinearGrid::SafeDownCast(output->GetPartitionedDataSet(nodeIds[0])->GetPartition(0));
  if (!mesh01)
  {
    std::cerr << "Mesh01 is nullptr" << std::endl;
    return EXIT_FAILURE;
  }

  if (!testValue(mesh01->GetNumberOfPoints(), 7056, "number of points in Mesh01"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(mesh01->GetNumberOfCells(), 6000, "number of points in Mesh01"))
  {
    return EXIT_FAILURE;
  }

  // Test Device HRR_3D
  nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("HRR_3D"));
  auto hrr3D =
    vtkPolyData::SafeDownCast(output->GetPartitionedDataSet(nodeIds[0])->GetPartition(0));
  if (!hrr3D)
  {
    std::cerr << "HRR_3D device is nullptr" << std::endl;
    return EXIT_FAILURE;
  }

  if (!testValue(hrr3D->GetNumberOfPoints(), 1, "number of points in HRR_3D"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(hrr3D->GetNumberOfCells(), 1, "number of points in HRR_3D"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(
        hrr3D->GetPointData()->GetArray("Value")->GetComponent(0, 0), 0.0, "value of HRR_3D"))
  {
    return EXIT_FAILURE;
  }

  // Test HRR
  nodeIds =
    outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("exemple_kitware_hrr"));
  auto hrr = vtkTable::SafeDownCast(output->GetPartitionAsDataObject(nodeIds[0], 0));
  if (!hrr)
  {
    std::cerr << "HRR is nullptr" << std::endl;
    return EXIT_FAILURE;
  }

  if (!testValue(hrr->GetRowData()->GetNumberOfArrays(), 13, "number of arrays in HRR table"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(
        hrr->GetRowData()->GetArray(0)->GetComponent(0, 0), 0.0, "value of array in HRR table"))
  {
    return EXIT_FAILURE;
  }

  // Test slice
  nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("VelX"));
  auto slice = vtkRectilinearGrid::SafeDownCast(output->GetPartition(nodeIds[0], 0));
  if (!slice)
  {
    std::cerr << "VelX slice is nullptr" << std::endl;
    return EXIT_FAILURE;
  }

  if (!testValue(slice->GetNumberOfPoints(), 441, "number of points in slice VelX"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(slice->GetNumberOfCells(), 400, "number of cells in slice VelX"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(
        slice->GetPointData()->GetArray("Values")->GetComponent(0, 0), 0.0, "value in VelX slice"))
  {
    return EXIT_FAILURE;
  }

  // Test boundary
  nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("Mesh01_Blockage_1"));
  auto boundary = vtkRectilinearGrid::SafeDownCast(output->GetPartition(nodeIds[0], 0));
  if (!boundary)
  {
    std::cerr << "Mesh01_Blockage_1 boundary is nullptr" << std::endl;
    return EXIT_FAILURE;
  }

  if (!testValue(
        boundary->GetNumberOfPoints(), 266, "number of points in Mesh01_Blockage_1 boundary"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(
        boundary->GetNumberOfCells(), 234, "number of cells in Mesh01_Blockage_1 boundary"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(std::abs(boundary->GetPointData()->GetArray("gauge")->GetComponent(0, 0)) < 1e-6,
        true, "gauge in Mesh01_Blockage_1 boundary"))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
