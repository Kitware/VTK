// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkDataAssembly.h>
#include <vtkFDSReader.h>
#include <vtkInformation.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRectilinearGrid.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTable.h>
#include <vtkTesting.h>

#include <cstdlib>

/**
 * This regression test ensures that empty slices names are correctly supported.
 * This scenario seems related to the version of FDS used to generate the test file.
 */

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

int TestFDSReader3(int argc, char* argv[])
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
  std::string fileName = dataRoot + "/Data/FDS/1000meca/1000meca.smv";
  reader->SetFileName(fileName);
  reader->UpdateInformation();

  vtkDataAssembly* assembly = reader->GetAssembly();
  if (!testValue(assembly->GetNumberOfChildren(0), 5, "number of root children"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(assembly->GetNumberOfChildren(1), 1, "number of grids"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(assembly->GetNumberOfChildren(2), 0, "number of devices"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(assembly->GetNumberOfChildren(3), 1, "number of hrr"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(assembly->GetNumberOfChildren(4), 2, "number of slices"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(assembly->GetNumberOfChildren(5), 26, "number of boundaries"))
  {
    return EXIT_FAILURE;
  }

  // Test extraction
  reader->AddSelector("/_1000meca/Grids");
  reader->AddSelector("/_1000meca/HRR");
  reader->AddSelector("/_1000meca/Slices/SOOT");
  reader->AddSelector("/_1000meca/Boundaries/Mesh01_Blockage_3");
  reader->Update();

  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutput());
  vtkDataAssembly* outAssembly = output->GetDataAssembly();

  if (!testValue(outAssembly->GetNumberOfChildren(0), 4, "number of root children"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(outAssembly->GetNumberOfChildren(1), 1, "number of grids"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(outAssembly->GetNumberOfChildren(2), 0, "number of devices"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(outAssembly->GetNumberOfChildren(3), 1, "number of hrr"))
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

  if (!testValue(mesh01->GetNumberOfPoints(), 468741, "number of points in Mesh01"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(mesh01->GetNumberOfCells(), 440000, "number of cells in Mesh01"))
  {
    return EXIT_FAILURE;
  }

  // Test HRR
  nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("_1000meca_hrr"));
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
  nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("SOOT"));
  auto slice = vtkRectilinearGrid::SafeDownCast(output->GetPartition(nodeIds[0], 0));
  if (!slice)
  {
    std::cerr << "Soot slice is nullptr" << std::endl;
    return EXIT_FAILURE;
  }

  if (!testValue(slice->GetNumberOfPoints(), 22321, "number of points in slice SOOT"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(slice->GetNumberOfCells(), 22000, "number of cells in slice SOOT"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(
        slice->GetPointData()->GetArray("Values")->GetComponent(0, 0), 0.0, "value in SOOT slice"))
  {
    return EXIT_FAILURE;
  }

  // Test boundary
  nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("Mesh01_Blockage_3"));
  auto boundary = vtkRectilinearGrid::SafeDownCast(output->GetPartition(nodeIds[0], 0));
  if (!boundary)
  {
    std::cerr << "Mesh01_Blockage_3 boundary is nullptr" << std::endl;
    return EXIT_FAILURE;
  }

  if (!testValue(
        boundary->GetNumberOfPoints(), 50, "number of points in Mesh01_Blockage_3 boundary"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(boundary->GetNumberOfCells(), 16, "number of cells in Mesh01_Blockage_3 boundary"))
  {
    return EXIT_FAILURE;
  }

  // Test number of timesteps
  auto outInfo = reader->GetOutputInformation(0);
  if (!outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    std::cerr << "Unable to retrieve timestep information " << std::endl;
    return EXIT_SUCCESS;
  }

  auto numberOfTimeSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (!testValue(numberOfTimeSteps, 21, "number of timesteps"))
  {
    return EXIT_SUCCESS;
  }

  return EXIT_SUCCESS;
}
