// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkCamera.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkIOSSReader.h>
#include <vtkNew.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>

#include "vtksys/SystemTools.hxx"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkCompositedSynchronizedRenderers.h"
#include "vtkMPIController.h"
#include "vtkSynchronizedRenderWindows.h"
#else
#include "vtkDummyController.h"
#endif

static std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

int TestIOSSCatalystExodus(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> contr;
#else
  vtkNew<vtkDummyController> contr;
#endif
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);
  const int numProcs = contr->GetNumberOfProcesses();
  const int myRank = contr->GetLocalProcessId();

  const std::string filename = "Data/Iocatalyst_can_ex2_MPI_" + std::to_string(numProcs);
  const std::string filepath = GetFileName(argc, argv, filename);
  vtksys::SystemTools::PutEnv("CATALYST_READER_TIME_STEP=0");
  vtksys::SystemTools::PutEnv("CATALYST_DATA_DUMP_DIRECTORY=" + filepath);
  vtkNew<vtkIOSSReader> reader;
  reader->SetFileName("catalyst.bin");
  reader->Update();

  vtkNew<vtkDataSetSurfaceFilter> surface;
  vtkNew<vtkCompositePolyDataMapper> mapper;
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;

  surface->SetInputConnection(reader->GetOutputPort());
  mapper->SetInputConnection(surface->GetOutputPort());
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SetPiece(myRank);
  mapper->SetNumberOfPieces(numProcs);
  mapper->SelectColorArray("ids");
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarRange(0, 10088 /*number of points*/);
  mapper->SetScalarVisibility(true);
  // update mapper an get parallel bounds.
  mapper->Update();
  vtkBoundingBox bbox(mapper->GetBounds());
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  contr->AllReduce(bbox, bbox);
#endif
  double bds[6];
  bbox.GetBounds(bds);

  actor->SetMapper(mapper);
  renWin->AddRenderer(ren);

  ren->AddActor(actor);
  renWin->SetSize(300, 300);
  auto cam = ren->GetActiveCamera();
  cam->SetPosition(10., 10., 5.);
  cam->SetViewUp(0., 0.4, 1.);
  ren->ResetCamera(bds);
  ren->ResetCameraClippingRange(bds);

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkSynchronizedRenderWindows> syncWindows;
  syncWindows->SetRenderWindow(renWin);
  syncWindows->SetParallelController(contr);
  syncWindows->SetIdentifier(1);

  vtkNew<vtkCompositedSynchronizedRenderers> syncRenderers;
  syncRenderers->SetRenderer(ren);
  syncRenderers->SetParallelController(contr);
#endif

  int retVal = EXIT_FAILURE;
  if (myRank == 0)
  {
    vtkNew<vtkRenderWindowInteractor> iren;
    iren->SetRenderWindow(renWin);
    iren->Initialize();
    retVal = vtkRegressionTestImage(renWin);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
    contr->TriggerBreakRMIs();
    contr->Broadcast(&retVal, 1, 0);
  }
  else
  {
    renWin->OffScreenRenderingOn();
    contr->ProcessRMIs();
    contr->Broadcast(&retVal, 1, 0);
  }
  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return !retVal;
}
