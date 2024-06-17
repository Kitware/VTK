// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkSpatioTemporalHarmonicsAttribute.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

int TestHDFWriterParallel(int argc, char* argv[])
{
  // Initialize MPI Controller
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  // Create a sphere source
  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(50);
  sphere->SetThetaResolution(50);

  // Create harmonics
  vtkNew<vtkSpatioTemporalHarmonicsAttribute> harmonics;
  harmonics->SetInputConnection(sphere->GetOutputPort());

  // Distribute it
  vtkNew<vtkRedistributeDataSetFilter> redistribute;
  redistribute->SetInputConnection(harmonics->GetOutputPort());

  // Retrieve temporary testing directory
  char* tempDirCStr =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tempDir{ tempDirCStr };
  delete[] tempDirCStr;
  std::string filePath = tempDir + "/parallel_sphere.vtkhdf";

  // Write it to disk
  vtkNew<vtkHDFWriter> writer;
  writer->SetInputConnection(redistribute->GetOutputPort());
  writer->SetFileName(filePath.c_str());
  writer->Write();

  // Reopen it and compare for every time step
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);

  vtkPolyData* readPiece = vtkPolyData::SafeDownCast(reader->GetOutputDataObject(0));
  vtkPolyData* originalPiece = vtkPolyData::SafeDownCast(redistribute->GetOutputDataObject(0));
  if (readPiece == nullptr)
  {
    vtkLog(ERROR, "Read piece should not be null");
    return EXIT_FAILURE;
  }

  if (!vtkTestUtilities::CompareDataObjects(readPiece, originalPiece))
  {
    vtkLog(ERROR, "Original and read piece do not match");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
