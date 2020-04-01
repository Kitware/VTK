/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIossExodusParitionedFiles.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Reads a partitioned exodus file in parallel
 */
#include <vtkBoundingBox.h>
#include <vtkCamera.h>
#include <vtkCompositePolyDataMapper2.h>
#include <vtkCompositedSynchronizedRenderers.h>
#include <vtkDataObject.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkIossReader.h>
#include <vtkNew.h>
#include <vtkProcessIdScalars.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSynchronizedRenderWindows.h>
#include <vtkTestUtilities.h>

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
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

int TestIossExodusParitionedFiles(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> contr;
#else
  vtkNew<vtkDummyController> contr;
#endif
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  const int myId = contr->GetLocalProcessId();
  const int numProcs = contr->GetNumberOfProcesses();

  vtkNew<vtkIossReader> reader;
  for (int cc = 0; cc < 4; ++cc)
  {
    auto fname =
      GetFileName(argc, argv, std::string("Data/Exodus/can.e.4/can.e.4.") + std::to_string(cc));
    reader->AddFileName(fname.c_str());
  }

  vtkNew<vtkProcessIdScalars> procIdScalars;
  vtkNew<vtkDataSetSurfaceFilter> surface;
  vtkNew<vtkCompositePolyDataMapper2> mapper;
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;

  procIdScalars->SetInputConnection(reader->GetOutputPort());
  procIdScalars->SetScalarModeToCellData();
  surface->SetInputConnection(procIdScalars->GetOutputPort());
  mapper->SetInputConnection(surface->GetOutputPort());
  mapper->SetPiece(myId);
  mapper->SetNumberOfPieces(numProcs);
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SetColorModeToMapScalars();
  mapper->SelectColorArray("ProcessId");
  mapper->SetScalarRange(0, numProcs - 1);

  // update mapper an get parallel bounds.
  mapper->Update();
  vtkBoundingBox bbox(mapper->GetBounds());
  contr->AllReduce(bbox, bbox);
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

  vtkNew<vtkSynchronizedRenderWindows> syncWindows;
  syncWindows->SetRenderWindow(renWin);
  syncWindows->SetParallelController(contr);
  syncWindows->SetIdentifier(1);

  vtkNew<vtkCompositedSynchronizedRenderers> syncRenderers;
  syncRenderers->SetRenderer(ren);
  syncRenderers->SetParallelController(contr);

  int retVal = EXIT_FAILURE;
  if (myId == 0)
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
