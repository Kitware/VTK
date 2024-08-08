// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataSetSurfaceFilter.h"
#include "vtkGenerateTimeSteps.h"
#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPassArrays.h"
#include "vtkPolyData.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkSpatioTemporalHarmonicsAttribute.h"
#include "vtkSphereSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"
#include "vtkWarpScalar.h"
#include "vtkXMLPolyDataReader.h"

namespace
{
bool TestDistributedObject(
  vtkMPIController* controller, const std::string& tempDir, bool usePolyData)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  // Create a sphere source
  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(50);
  sphere->SetThetaResolution(50);

  // Distribute it
  vtkNew<vtkRedistributeDataSetFilter> redistribute;
  redistribute->SetGenerateGlobalCellIds(false);
  redistribute->SetInputConnection(sphere->GetOutputPort());

  // Extract surface to get a poly data again
  vtkNew<vtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(redistribute->GetOutputPort());

  // Write it to disk
  std::string prefix = tempDir + "/parallel_sphere_" + (usePolyData ? "PD" : "UG");
  std::string filePath = prefix + ".vtkhdf";
  std::string filePathPart = prefix + "_part" + std::to_string(myRank) + ".vtkhdf";

  {
    vtkNew<vtkHDFWriter> writer;
    writer->SetInputConnection(
      usePolyData ? surface->GetOutputPort() : redistribute->GetOutputPort());
    writer->SetFileName(filePath.c_str());
    writer->Write();
  }

  // Wait for all processes to be done writing
  controller->Barrier();

  // Reopen file and compare it to the source
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);

  vtkNew<vtkHDFReader> readerPart;
  readerPart->SetFileName(filePathPart.c_str());
  readerPart->Update();

  vtkDataObject* readPiece = reader->GetOutputDataObject(0);
  vtkDataObject* originalPiece =
    usePolyData ? surface->GetOutputDataObject(0) : redistribute->GetOutputDataObject(0);
  vtkDataObject* readPart = readerPart->GetOutputDataObject(0);

  if (!vtkTestUtilities::CompareDataObjects(readPiece, originalPiece))
  {
    vtkLog(ERROR, "Original and read piece do not match");
    return false;
  }

  if (!vtkTestUtilities::CompareDataObjects(readPiece, readPart))
  {
    vtkLog(ERROR, "Read piece and read part do not match");
    return false;
  }

  return true;
}

/**
 * Pipeline used for this test:
 * Cow > Redistribute > (usePolyData ? SurfaceFilter ) > Generate Time steps > Harmonics >
 * (!staticMesh ? warp by scalar) > Pass arrays > VTKHDF Writer > Read whole/part
 *
 * No animals were harmed in the making of this test.
 */
bool TestDistributedTemporal(vtkMPIController* controller, const std::string& tempDir,
  const std::string& dataRoot, bool usePolyData, bool staticMesh)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  const std::string basePath = dataRoot + "/Data/cow.vtp";
  vtkNew<vtkXMLPolyDataReader> baseReader;
  baseReader->SetFileName(basePath.c_str());

  // Redistribute cow
  vtkNew<vtkRedistributeDataSetFilter> redistribute;
  redistribute->SetGenerateGlobalCellIds(true);
  redistribute->SetInputConnection(baseReader->GetOutputPort());

  // Extract surface to get a poly data again
  vtkNew<vtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(redistribute->GetOutputPort());

  // Generate several time steps
  vtkNew<vtkGenerateTimeSteps> generateTimeSteps;
  const std::vector<double> timeValues{ 1.0, 3.0, 5.0 };
  for (const double& value : timeValues)
  {
    generateTimeSteps->AddTimeStepValue(value);
  }
  generateTimeSteps->SetInputConnection(
    usePolyData ? surface->GetOutputPort() : redistribute->GetOutputPort());

  // Generate a time-varying point field: use default ParaView weights
  vtkNew<vtkSpatioTemporalHarmonicsAttribute> harmonics;
  harmonics->AddHarmonic(1.0, 1.0, 0.6283, 0.6283, 0.6283, 0.0);
  harmonics->AddHarmonic(3.0, 1.0, 0.6283, 0.0, 0.0, 1.5708);
  harmonics->AddHarmonic(2.0, 2.0, 0.0, 0.6283, 0.0, 3.1416);
  harmonics->AddHarmonic(1.0, 3.0, 0.0, 0.0, 0.6283, 4.7124);
  harmonics->SetInputConnection(generateTimeSteps->GetOutputPort());

  // Warp by scalar
  vtkNew<vtkWarpScalar> warp;
  warp->SetInputConnection(harmonics->GetOutputPort());

  // Write data in parallel to disk
  std::string prefix =
    tempDir + "/parallel_time_cow" + (usePolyData ? "_PD" : "_UG") + (staticMesh ? "_static" : "");
  std::string filePath = prefix + ".vtkhdf";
  std::string filePathPart = prefix + "_part" + std::to_string(myRank) + ".vtkhdf";

  {
    vtkNew<vtkHDFWriter> writer;
    writer->SetInputConnection(staticMesh ? harmonics->GetOutputPort() : warp->GetOutputPort());
    writer->SetWriteAllTimeSteps(true);
    writer->SetFileName(filePath.c_str());
    writer->SetDebug(true);
    writer->Write();
  }

  // All processes have written their pieces to disk
  controller->Barrier();

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);

  vtkNew<vtkHDFReader> readerPart;
  readerPart->SetFileName(filePathPart.c_str());
  readerPart->Update();

  for (int time = 0; time < static_cast<int>(timeValues.size()); time++)
  {
    vtkDebugWithObjectMacro(nullptr, << "Comparing timestep " << time);

    reader->SetStep(time);
    reader->UpdatePiece(myRank, nbRanks, 0);

    readerPart->SetStep(time);
    readerPart->Update();

    if (usePolyData)
    {
      vtkPolyData* readPiece = vtkPolyData::SafeDownCast(reader->GetOutputDataObject(0));
      vtkPolyData* readPart = vtkPolyData::SafeDownCast(readerPart->GetOutputDataObject(0));

      if (readPiece == nullptr || readPart == nullptr)
      {
        vtkLog(ERROR, "Piece should not be null");
        return false;
      }
    }
    else
    {
      vtkUnstructuredGrid* readPiece =
        vtkUnstructuredGrid::SafeDownCast(reader->GetOutputDataObject(0));
      vtkUnstructuredGrid* readPart =
        vtkUnstructuredGrid::SafeDownCast(readerPart->GetOutputDataObject(0));

      if (readPiece == nullptr || readPart == nullptr)
      {
        vtkLog(ERROR, "Piece should not be null");
        return false;
      }
    }

    vtkDataObject* readPiece = reader->GetOutputDataObject(0);
    vtkDataObject* readPart = readerPart->GetOutputDataObject(0);

    if (!vtkTestUtilities::CompareDataObjects(readPiece, readPart))
    {
      vtkLog(ERROR, "Read piece and read part do not match");
      return false;
    }
  }

  return true;
}

bool TestDistributedPolyData(vtkMPIController* controller, const std::string& tempDir)
{
  return TestDistributedObject(controller, tempDir, true);
}
bool TestDistributedUnstructuredGrid(vtkMPIController* controller, const std::string& tempDir)
{
  return TestDistributedObject(controller, tempDir, false);
}
bool TestDistributedUnstructuredGridTemporal(
  vtkMPIController* controller, const std::string& tempDir, const std::string& dataRoot)
{
  return TestDistributedTemporal(controller, tempDir, dataRoot, false, false);
}
bool TestDistributedUnstructuredGridTemporalStatic(
  vtkMPIController* controller, const std::string& tempDir, const std::string& dataRoot)
{
  return TestDistributedTemporal(controller, tempDir, dataRoot, false, true);
}
bool TestDistributedPolyDataTemporal(
  vtkMPIController* controller, const std::string& tempDir, const std::string& dataRoot)
{
  return TestDistributedTemporal(controller, tempDir, dataRoot, true, false);
}
bool TestDistributedPolyDataTemporalStatic(
  vtkMPIController* controller, const std::string& tempDir, const std::string& dataRoot)
{
  return TestDistributedTemporal(controller, tempDir, dataRoot, true, true);
}

}

int TestHDFWriterDistributed(int argc, char* argv[])
{
  // Initialize MPI Controller
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  std::string threadName = "rank #";
  threadName += std::to_string(controller->GetLocalProcessId());
  vtkLogger::SetThreadName(threadName);

  // Retrieve temporary testing directory
  char* tempDirCStr =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tempDir{ tempDirCStr };
  delete[] tempDirCStr;

  // Get data directory
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified." << std::endl;
    return EXIT_FAILURE;
  }
  std::string dataRoot = testHelper->GetDataRoot();

  bool res = true;
  res &= ::TestDistributedPolyData(controller, tempDir);
  res &= ::TestDistributedUnstructuredGrid(controller, tempDir);
  res &= ::TestDistributedUnstructuredGridTemporal(controller, tempDir, dataRoot);
  res &= ::TestDistributedUnstructuredGridTemporalStatic(controller, tempDir, dataRoot);
  res &= ::TestDistributedPolyDataTemporal(controller, tempDir, dataRoot);
  res &= ::TestDistributedPolyDataTemporalStatic(controller, tempDir, dataRoot);
  controller->Finalize();
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
