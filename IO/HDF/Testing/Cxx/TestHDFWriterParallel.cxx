// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPassArrays.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkSpatioTemporalHarmonicsAttribute.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

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
}

int TestHDFWriterParallel(int argc, char* argv[])
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

  bool res = ::TestParallelUnstrucutredGrid(controller, tempDir);
  controller->Finalize();
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
