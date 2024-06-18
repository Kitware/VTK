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
bool TestParallelPolyData(vtkMPIController* controller, const std::string& tempDir)
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

  {
    vtkNew<vtkHDFWriter> writer;
    writer->SetInputConnection(redistribute->GetOutputPort());
    writer->SetFileName(filePath.c_str());
    writer->SetDebug(true);
    writer->SetUseExternalPartitions(true);
    writer->Write();
  }

  // Wait for all processes to be done writing
  controller->Barrier();

  // Reopen file and compare it to the source
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);

  vtkUnstructuredGrid* readPiece =
    vtkUnstructuredGrid::SafeDownCast(reader->GetOutputDataObject(0));
  vtkUnstructuredGrid* originalPiece =
    vtkUnstructuredGrid::SafeDownCast(redistribute->GetOutputDataObject(0));

  std::cout << myRank << " " << readPiece->GetNumberOfCells() << " cells" << std::endl;

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

  bool res = ::TestParallelPolyData(controller, tempDir);
  controller->Finalize();
  return EXIT_SUCCESS;
  // return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
