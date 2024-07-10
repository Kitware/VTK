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
 * This regression test ensures that slices names with a a space between the name and the
 * name delimiter (% or # symbol) are correctly supported.
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

int TestFDSReader2(int argc, char* argv[])
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
  std::string fileName = dataRoot + "/Data/FDS/visibility_adjustment/visibility_adjustment.smv";
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
  if (!testValue(assembly->GetNumberOfChildren(4), 1, "number of slices"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(assembly->GetNumberOfChildren(5), 0, "number of boundaries"))
  {
    return EXIT_FAILURE;
  }

  // Test extraction
  reader->AddSelector("/visibility_adjustment/Grids/MESH_0000001");
  reader->AddSelector("/visibility_adjustment/HRR/visibility_adjustment_hrr");
  reader->AddSelector("/visibility_adjustment/Slices/STRUCTURED_SOOT");
  reader->Update();

  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutput());
  vtkDataAssembly* outAssembly = output->GetDataAssembly();

  if (!testValue(outAssembly->GetNumberOfChildren(0), 3, "number of root children"))
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
  if (!testValue(outAssembly->GetNumberOfChildren(5), 0, "number of boundaries"))
  {
    return EXIT_FAILURE;
  }

  // Test Mesh01
  auto nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("MESH_0000001"));
  auto mesh01 =
    vtkRectilinearGrid::SafeDownCast(output->GetPartitionedDataSet(nodeIds[0])->GetPartition(0));
  if (!mesh01)
  {
    std::cerr << "MESH_0000001 is nullptr" << std::endl;
    return EXIT_FAILURE;
  }

  if (!testValue(mesh01->GetNumberOfPoints(), 6615, "number of points in MESH_0000001"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(mesh01->GetNumberOfCells(), 5600, "number of cells in MESH_0000001"))
  {
    return EXIT_FAILURE;
  }

  // Test HRR
  nodeIds =
    outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("visibility_adjustment_hrr"));
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
  nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("STRUCTURED_SOOT"));
  auto slice = vtkRectilinearGrid::SafeDownCast(output->GetPartition(nodeIds[0], 0));
  if (!slice)
  {
    std::cerr << "Soot slice is nullptr" << std::endl;
    return EXIT_FAILURE;
  }

  if (!testValue(slice->GetNumberOfPoints(), 441, "number of points in slice SOOT"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(slice->GetNumberOfCells(), 400, "number of cells in slice SOOT"))
  {
    return EXIT_FAILURE;
  }

  if (!testValue(
        slice->GetPointData()->GetArray("Values")->GetComponent(0, 0), 30.0, "value in SOOT slice"))
  {
    return EXIT_FAILURE;
  }

  // Test number of timesteps
  auto outInfo = reader->GetOutputInformation(0);
  if (!outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    std::cerr << "Unable to retrieve timestep information " << std::endl;
    return EXIT_FAILURE;
  }

  auto numberOfTimeSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (!testValue(numberOfTimeSteps, 1001, "number of timesteps"))
  {
    return EXIT_FAILURE;
  }

  reader->UpdateTimeStep(31.0);
  output = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutput());
  outAssembly = output->GetDataAssembly();
  nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("STRUCTURED_SOOT"));
  slice = vtkRectilinearGrid::SafeDownCast(output->GetPartition(nodeIds[0], 0));
  if (!testValue(
        std::abs(slice->GetPointData()->GetArray("Values")->GetComponent(259, 0) - 3.29237) < 1e-5,
        true, "soot at time value 31"))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
