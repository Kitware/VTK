// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkCellData.h>
#include <vtkDataAssembly.h>
#include <vtkFDSReader.h>
#include <vtkInformation.h>
#include <vtkMathUtilities.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRectilinearGrid.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTable.h>
#include <vtkTesting.h>

#include <cstdlib>
#include <fstream>

/**
 * This tests the core features of the FDS reader, i.e. the parsing of:
 * - Grid
 * - Devices & HRR
 * - Slices
 * - Boundaries
 *
 * When adding new features, please consider updating the associated data
 * (edit the test_core.fds file and re-run the simulation) instead of adding
 * new files when possible.
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

template <typename T1, typename T2>
bool testValueFuzzy(T1 gotVal, T2 expectedVal, const char* valName)
{
  if (!vtkMathUtilities::FuzzyCompare(gotVal, expectedVal))
  {
    std::cerr << "Wrong " << valName << ". Expected " << expectedVal << ", got " << gotVal
              << std::endl;
    return false;
  }
  return true;
}
}

bool TestEmptyFile(const std::string& tempDirectory)
{
  // Create empty file.
  std::string emptyFilePath = tempDirectory + "/empty.smv";
  std::ofstream outputStream(emptyFilePath);
  outputStream.close();

  vtkNew<vtkFDSReader> reader;
  reader->SetFileName(emptyFilePath);

  auto previousGlobalWarningFlag = vtkObject::GetGlobalWarningDisplay();
  vtkObject::SetGlobalWarningDisplay(0);
  reader->UpdateTimeStep(0.0);
  vtkObject::SetGlobalWarningDisplay(previousGlobalWarningFlag);

  return true;
}

bool TestExampleFile(const std::string& dataRoot)
{
  // Test RequestInformation
  vtkNew<vtkFDSReader> reader;
  std::string fileName = dataRoot + "/Data/FDS/test_core/test_core.smv";
  reader->SetFileName(fileName);
  reader->UpdateInformation();

  vtkDataAssembly* assembly = reader->GetAssembly();
  if (!testValue(assembly->GetNumberOfChildren(0), 5, "number of root children"))
  {
    return false;
  }
  if (!testValue(assembly->GetNumberOfChildren(1), 2, "number of grids"))
  {
    return false;
  }
  if (!testValue(assembly->GetNumberOfChildren(2), 4, "number of devices"))
  {
    return false;
  }
  if (!testValue(assembly->GetNumberOfChildren(3), 1, "number of hrr"))
  {
    return false;
  }
  if (!testValue(assembly->GetNumberOfChildren(4), 11, "number of slices"))
  {
    return false;
  }
  if (!testValue(assembly->GetNumberOfChildren(5), 12, "number of boundaries"))
  {
    return false;
  }

  // Test extraction
  reader->AddSelector("/test_core/Grids/Mesh01");
  reader->AddSelector("/test_core/Devices/HRR_3D");
  reader->AddSelector("/test_core/HRR/test_core_hrr");
  reader->AddSelector("/test_core/Slices/STRUCTURED_VelX_VELOCITY");
  // Following slice contains cell-centered data
  reader->AddSelector("/test_core/Slices/STRUCTURED_TempZ_TEMPERATURE");
  reader->AddSelector("/test_core/Boundaries/Mesh01_Blockage_3");
  reader->Update();

  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutput());
  vtkDataAssembly* outAssembly = output->GetDataAssembly();

  if (!testValue(outAssembly->GetNumberOfChildren(0), 5, "number of root children"))
  {
    return false;
  }
  if (!testValue(outAssembly->GetNumberOfChildren(1), 1, "number of grids"))
  {
    return false;
  }
  if (!testValue(outAssembly->GetNumberOfChildren(2), 1, "number of devices"))
  {
    return false;
  }
  if (!testValue(outAssembly->GetNumberOfChildren(3), 1, "number of hrrs"))
  {
    return false;
  }
  // XXX: STRUCTURED_TempZ_TEMPERATURE covers 2 grids, resulting on having 3 slices.
  // See https://gitlab.kitware.com/paraview/paraview/-/issues/22683
  if (!testValue(outAssembly->GetNumberOfChildren(4), 3, "number of slices"))
  {
    return false;
  }
  if (!testValue(outAssembly->GetNumberOfChildren(5), 1, "number of boundaries"))
  {
    return false;
  }

  // Test Mesh01
  auto nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("Mesh01"));
  auto* mesh01 =
    vtkRectilinearGrid::SafeDownCast(output->GetPartitionedDataSet(nodeIds[0])->GetPartition(0));
  if (!mesh01)
  {
    std::cerr << "Mesh01 is nullptr" << std::endl;
    return false;
  }

  if (!testValue(mesh01->GetNumberOfPoints(), 7056, "number of points in Mesh01"))
  {
    return false;
  }

  if (!testValue(mesh01->GetNumberOfCells(), 6000, "number of points in Mesh01"))
  {
    return false;
  }

  // Test Device HRR_3D
  nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("HRR_3D"));
  auto* hrr3D =
    vtkPolyData::SafeDownCast(output->GetPartitionedDataSet(nodeIds[0])->GetPartition(0));
  if (!hrr3D)
  {
    std::cerr << "HRR_3D device is nullptr" << std::endl;
    return false;
  }

  if (!testValue(hrr3D->GetNumberOfPoints(), 1, "number of points in HRR_3D"))
  {
    return false;
  }

  if (!testValue(hrr3D->GetNumberOfCells(), 1, "number of points in HRR_3D"))
  {
    return false;
  }

  if (!testValue(
        hrr3D->GetPointData()->GetArray("Value")->GetComponent(0, 0), 0.0, "value of HRR_3D"))
  {
    return false;
  }

  // Test HRR
  nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("test_core_hrr"));
  auto* hrr = vtkTable::SafeDownCast(output->GetPartitionAsDataObject(nodeIds[0], 0));
  if (!hrr)
  {
    std::cerr << "HRR is nullptr" << std::endl;
    return false;
  }

  if (!testValue(hrr->GetRowData()->GetNumberOfArrays(), 13, "number of arrays in HRR table"))
  {
    return false;
  }

  if (!testValue(
        hrr->GetRowData()->GetArray(0)->GetComponent(0, 0), 0.0, "value of array in HRR table"))
  {
    return false;
  }

  // Test slice with point-centered data
  nodeIds =
    outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("STRUCTURED_VelX_VELOCITY"));
  auto* sliceVelX = vtkRectilinearGrid::SafeDownCast(output->GetPartition(nodeIds[0], 0));
  if (!sliceVelX)
  {
    std::cerr << "VelX slice is nullptr" << std::endl;
    return false;
  }

  if (!testValue(sliceVelX->GetNumberOfPoints(), 441, "number of points in sliceVelX"))
  {
    return false;
  }

  if (!testValue(sliceVelX->GetNumberOfCells(), 400, "number of cells in sliceVelX"))
  {
    return false;
  }

  if (!sliceVelX->GetPointData()->GetArray("Values"))
  {
    std::cerr << "VelX slice has no \"Values\" point data array." << std::endl;
    return false;
  }

  if (!testValue(sliceVelX->GetPointData()->GetArray("Values")->GetComponent(0, 0), 0.0,
        "value in VelX slice"))
  {
    return false;
  }

  // Test slice with cell-centered data
  nodeIds = outAssembly->GetDataSetIndices(
    outAssembly->FindFirstNodeWithName("STRUCTURED_TempZ_TEMPERATURE"));
  auto* sliceTempZ = vtkRectilinearGrid::SafeDownCast(output->GetPartition(nodeIds[0], 0));
  if (!sliceTempZ)
  {
    std::cerr << "TempZ slice is nullptr" << std::endl;
    return false;
  }

  // XXX: Slice TempZ covering first grid only has only 336 points.
  // Total considering two parts (covering both grids) is 462.
  // See https://gitlab.kitware.com/paraview/paraview/-/issues/22683
  if (!testValue(sliceTempZ->GetNumberOfPoints(), 336, "number of points in slice TempZ"))
  {
    return false;
  }

  // XXX: Slice TempZ covering first grid only has only 300 cells.
  // Total considering two parts (covering both grids) is 400.
  // See https://gitlab.kitware.com/paraview/paraview/-/issues/22683
  if (!testValue(sliceTempZ->GetNumberOfCells(), 300, "number of cells in slice TempZ"))
  {
    return false;
  }

  if (!sliceTempZ->GetCellData()->GetArray("Values"))
  {
    std::cerr << "TempZ slice has no \"Values\" cell data array." << std::endl;
    return false;
  }

  if (!testValue(sliceTempZ->GetCellData()->GetArray("Values")->GetComponent(0, 0), 20.0,
        "value in TempZ slice"))
  {
    return false;
  }

  // Test boundary
  nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("Mesh01_Blockage_3"));
  auto* boundary = vtkRectilinearGrid::SafeDownCast(output->GetPartition(nodeIds[0], 0));
  if (!boundary)
  {
    std::cerr << "Mesh01_Blockage_3 boundary is nullptr" << std::endl;
    return false;
  }

  if (!testValue(
        boundary->GetNumberOfPoints(), 266, "number of points in Mesh01_Blockage_3 boundary"))
  {
    return false;
  }

  if (!testValue(
        boundary->GetNumberOfCells(), 234, "number of cells in Mesh01_Blockage_3 boundary"))
  {
    return false;
  }

  if (!boundary->GetPointData()->GetArray("gauge"))
  {
    std::cerr << "Mesh01_Blockage_3 boundary has no \"gauge\" point data array." << std::endl;
    return false;
  }

  const double value_at_t0 = -0.00013127682905178517103195190429688;
  if (!testValueFuzzy(boundary->GetPointData()->GetArray("gauge")->GetComponent(0, 0), value_at_t0,
        "gauge in Mesh01_Blockage_3 boundary"))
  {
    return false;
  }

  if (!boundary->GetCellData()->GetArray("gauge"))
  {
    std::cerr << "Mesh01_Blockage_3 boundary has no \"gauge\" point data array." << std::endl;
    return false;
  }

  // Same value than before since no interpolation is done on the corner of the boundary
  if (!testValueFuzzy(boundary->GetCellData()->GetArray("gauge")->GetComponent(0, 0), value_at_t0,
        "gauge in Mesh01_Blockage_3 boundary"))
  {
    return false;
  }

  // Test number of timesteps
  auto* outInfo = reader->GetOutputInformation(0);
  if (!outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    std::cerr << "Unable to retrieve timestep information " << std::endl;
    return false;
  }

  auto numberOfTimeSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (!testValue(numberOfTimeSteps, 31, "number of timesteps"))
  {
    return false;
  }

  // Now update timestep
  reader->UpdateTimeStep(8.1);

  output = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutput());
  outAssembly = output->GetDataAssembly();
  nodeIds = outAssembly->GetDataSetIndices(outAssembly->FindFirstNodeWithName("Mesh01_Blockage_3"));
  boundary = vtkRectilinearGrid::SafeDownCast(output->GetPartition(nodeIds[0], 0));

  const double value_at_t8 = 0.935839116573333740234375;
  if (!testValueFuzzy(boundary->GetPointData()->GetArray("gauge")->GetComponent(0, 0), value_at_t8,
        "gauge in Mesh01_Blockage_3 boundary at time value 8.1"))
  {
    return false;
  }
  if (!testValueFuzzy(boundary->GetCellData()->GetArray("gauge")->GetComponent(0, 0), value_at_t8,
        "gauge (cell-centered) in Mesh01_Blockage_3 boundary at time value 8.1"))
  {
    return false;
  }

  return true;
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
  if (!TestExampleFile(dataRoot))
  {
    return EXIT_FAILURE;
  }

  std::string tempDirectory = testHelper->GetTempDirectory();
  if (!TestEmptyFile(tempDirectory))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
