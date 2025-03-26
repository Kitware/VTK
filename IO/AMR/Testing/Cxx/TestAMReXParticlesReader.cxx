// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <cstdlib>
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "vtkAMReXParticlesReader.h"
#include "vtkDataArraySelection.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"

#define ensure(x, msg)                                                                             \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      cerr << "FAILED: " << msg << endl;                                                           \
      controller->Finalize();                                                                      \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

int Validate(vtkMultiBlockDataSet* mb)
{
  auto* controller = vtkMultiProcessController::GetGlobalController();
  ensure(mb != nullptr, "expecting vtkMultiBlockDataSet.");
  ensure(mb->GetNumberOfBlocks() == 1, "expecting num-blocks == num-levels == 1");

  auto mp = vtkMultiPieceDataSet::SafeDownCast(mb->GetBlock(0));
  ensure(mp != nullptr, "expecting level is maintained in a vtkMultiPieceDataSet.");
  ensure(mp->GetNumberOfPieces() == 8, "expecting 8 datasets in level 0");
  vtkIdType numberOfPointsPerProcess = 0;
  for (unsigned int cc = 0; cc < mp->GetNumberOfPieces(); ++cc)
  {
    if (auto pd = vtkPolyData::SafeDownCast(mp->GetPiece(cc)))
    {
      ensure(pd != nullptr, "expecting polydata for index " << cc);
      numberOfPointsPerProcess += pd->GetNumberOfPoints();
      ensure(numberOfPointsPerProcess > 0, "expecting non-null points.");
      ensure(pd->GetPointData()->GetArray("density") != nullptr, "missing density");
    }
  }
  vtkIdType totalNumberOfPoints = 0;
  controller->AllReduce(
    &numberOfPointsPerProcess, &totalNumberOfPoints, 1, vtkCommunicator::SUM_OP);
  if (totalNumberOfPoints != 9776)
  {
    vtkLog(ERROR, << "# points per process: " << numberOfPointsPerProcess);
    vtkLog(ERROR, << "Expected total # points: 9776");
    vtkLog(ERROR, << "Got total # points: " << totalNumberOfPoints);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int TestAMReXParticlesReader(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> controller;
#else
  vtkNew<vtkDummyController> controller;
#endif
  controller->Initialize(&argc, &argv);
  const int processId = controller->GetLocalProcessId();
  const int numberOfProcesses = controller->GetNumberOfProcesses();
  vtkLogger::SetThreadName("processId=" + std::to_string(processId));
  vtkMultiProcessController::SetGlobalController(controller);
  // Test 3D
  {
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMReX/MFIX-Exa/plt00000");
    vtkNew<vtkAMReXParticlesReader> reader;
    reader->SetPlotFileName(fname);
    delete[] fname;

    reader->SetParticleType("particles");
    reader->GetPointDataArraySelection()->DisableArray("proc");
    reader->UpdateInformation();
    ensure(reader->GetPointDataArraySelection()->ArrayIsEnabled("proc") == 0,
      "`proc` should be disabled.");
    reader->UpdatePiece(processId, numberOfProcesses, 0);
    if (Validate(reader->GetOutput()) == EXIT_FAILURE)
    {
      controller->Finalize();
      return EXIT_FAILURE;
    }
  }

  // Test 2D
  {
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMReX/Sample2D/plt00100");
    vtkNew<vtkAMReXParticlesReader> reader;
    reader->SetPlotFileName(fname);
    delete[] fname;

    reader->SetParticleType("Tracer");
    reader->UpdateInformation();
    reader->Update();

    double bds[6];
    reader->GetOutput()->GetBounds(bds);
    ensure(bds[4] == bds[5], "expecting 2D dataset");
  }

  controller->Finalize();
  return EXIT_SUCCESS;
}
