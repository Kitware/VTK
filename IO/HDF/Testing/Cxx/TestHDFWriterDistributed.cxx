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
#include "vtkXMLPolyDataReader.h"

namespace
{
bool TestParallelUnstrucutredGrid(vtkMPIController* controller, const std::string& tempDir)
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

  // Write it to disk
  std::string filePath = tempDir + "/parallel_sphere.vtkhdf";
  std::string filePathPart = tempDir + "/parallel_sphere_part" + std::to_string(myRank) + ".vtkhdf";

  {
    vtkNew<vtkHDFWriter> writer;
    writer->SetInputConnection(redistribute->GetOutputPort());
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

  vtkUnstructuredGrid* readPiece =
    vtkUnstructuredGrid::SafeDownCast(reader->GetOutputDataObject(0));
  vtkUnstructuredGrid* originalPiece =
    vtkUnstructuredGrid::SafeDownCast(redistribute->GetOutputDataObject(0));
  vtkUnstructuredGrid* readPart =
    vtkUnstructuredGrid::SafeDownCast(readerPart->GetOutputDataObject(0));

  if (readPiece == nullptr || originalPiece == nullptr || readPart == nullptr)
  {
    vtkLog(ERROR, "Piece should not be null");
    return false;
  }

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

bool TestParallelTemporalPolyData(
  vtkMPIController* controller, const std::string& tempDir, const std::string& dataRoot)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  const std::string basePath = dataRoot + "/Data/cow.vtp";
  vtkNew<vtkXMLPolyDataReader> baseReader;
  baseReader->SetFileName(basePath.c_str());

  // Redistribute cow
  vtkNew<vtkRedistributeDataSetFilter> redistribute;
  redistribute->SetGenerateGlobalCellIds(false);
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
  generateTimeSteps->SetInputConnection(surface->GetOutputPort());

  // Generate a time-varying point field
  vtkNew<vtkSpatioTemporalHarmonicsAttribute> harmonics;
  harmonics->AddHarmonic(1.0, 1.0, 0.6283, 0.6283, 0.6283, 0.0);
  harmonics->AddHarmonic(3.0, 1.0, 0.6283, 0.0, 0.0, 1.5708);
  harmonics->AddHarmonic(2.0, 2.0, 0.0, 0.6283, 0.0, 3.1416);
  harmonics->AddHarmonic(1.0, 3.0, 0.0, 0.0, 0.6283, 4.7124);
  harmonics->SetInputConnection(generateTimeSteps->GetOutputPort());

  // Write data in parallel to disk
  std::string filePath = tempDir + "/parallel_time_cow.vtkhdf";

  {
    vtkNew<vtkHDFWriter> writer;
    writer->SetInputConnection(harmonics->GetOutputPort());
    writer->SetWriteAllTimeSteps(false); // TODO: When Temporal + Distributed is fully supported,
                                         // this test will need to evolve, especially this line.
    writer->SetFileName(filePath.c_str());
    writer->Write();
  }

  // All processes write their pieces to disk
  controller->Barrier();

  // Read and compare each timestep
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);

  vtkPolyData* readPiece = vtkPolyData::SafeDownCast(reader->GetOutputDataObject(0));
  vtkPolyData* originalPiece = vtkPolyData::SafeDownCast(harmonics->GetOutputDataObject(0));

  if (readPiece == nullptr || originalPiece == nullptr)
  {
    vtkLog(ERROR, "Piece should not be null");
    return false;
  }

  if (!vtkTestUtilities::CompareDataObjects(readPiece, originalPiece))
  {
    vtkLog(ERROR, "Original and read piece do not match");
    return false;
  }

  return true;
}

}

int TestHDFWriterDistributed(int argc, char* argv[])
{
  // Initialize MPI Controller
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

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

  bool res = ::TestParallelUnstrucutredGrid(controller, tempDir);
  res &= ::TestParallelTemporalPolyData(controller, tempDir, dataRoot);
  controller->Finalize();
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
