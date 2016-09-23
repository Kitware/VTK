/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProcess.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <mpi.h>

#include "vtkMPIController.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkLineSource.h"
#include "vtkPDataSetReader.h"
#include "vtkPProbeFilter.h"
#include "vtkPOutlineFilter.h"
#include "vtkTestUtilities.h"
#include "vtkProperty.h"
#include "vtkTubeFilter.h"
#include "vtkCompositeRenderManager.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"


int TestPProbe(int argc,char *argv[])
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc,&argv);

  vtkMPIController* contr = vtkMPIController::New();
  contr->Initialize();

  int numProcs =contr->GetNumberOfProcesses();
  int me=contr->GetLocalProcessId();

// create a rendering window and renderer
  vtkRenderer* Ren1 = vtkRenderer::New();
  Ren1->SetBackground(.5,.8,1);

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(Ren1);
  renWin->SetSize(300,300);

  if (me > 0)
  {
    renWin->SetPosition(me*350, 0);
    renWin->OffScreenRenderingOn();
  }

// camera parameters
  vtkCamera* camera = Ren1->GetActiveCamera();
  camera->SetPosition(199.431,196.879,15.7781);
  camera->SetFocalPoint(33.5,33.5,33.5);
  camera->SetViewUp(0.703325,-0.702557,0.108384);
  camera->SetViewAngle(30);
  camera->SetClippingRange(132.14,361.741);

  vtkPDataSetReader* ironProt0 = vtkPDataSetReader::New();
  char* fname1 = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                      "Data/ironProt.vtk");
  ironProt0->SetFileName(fname1);
  delete[] fname1;

  vtkPOutlineFilter* Geometry4 = vtkPOutlineFilter::New();
  Geometry4->SetController(contr);
  Geometry4->SetInputConnection(ironProt0->GetOutputPort());

  vtkPolyDataMapper* Mapper4 = vtkPolyDataMapper::New();
  Mapper4->SetInputConnection(Geometry4->GetOutputPort());
  Mapper4->SetImmediateModeRendering(0);
  Mapper4->SetScalarRange(0,1);
  Mapper4->SetScalarVisibility(0);
  Mapper4->SetScalarModeToDefault();

  vtkActor* Actor4 = vtkActor::New();
  Actor4->SetMapper(Mapper4);
  Actor4->GetProperty()->SetRepresentationToSurface();
  Actor4->GetProperty()->SetInterpolationToGouraud();
  Actor4->GetProperty()->SetColor(1,1,1);
  Ren1->AddActor(Actor4);

  vtkLineSource* probeLine = vtkLineSource::New();
  probeLine->SetPoint1(0,67,10);
  probeLine->SetPoint2(67,0,50);
  probeLine->SetResolution(500);

  vtkPProbeFilter* Probe0 = vtkPProbeFilter::New();
  Probe0->SetSourceConnection(ironProt0->GetOutputPort());
  Probe0->SetInputConnection(probeLine->GetOutputPort());
  Probe0->SetController(contr);

  vtkTubeFilter* Tuber0 = vtkTubeFilter::New();
  Tuber0->SetInputConnection(Probe0->GetOutputPort());
  Tuber0->SetNumberOfSides(10);
  Tuber0->SetCapping(0);
  Tuber0->SetRadius(1);
  Tuber0->SetVaryRadius(1);
  Tuber0->SetRadiusFactor(10);
  Tuber0->Update();

  vtkPolyDataMapper* Mapper6 = vtkPolyDataMapper::New();
  Mapper6->SetInputConnection(Tuber0->GetOutputPort());
  Mapper6->SetImmediateModeRendering(0);
  Mapper6->SetScalarRange(0,228);
  Mapper6->SetScalarVisibility(1);
  Mapper6->SetScalarModeToUsePointFieldData();
  Mapper6->ColorByArrayComponent("scalars",-1);
  Mapper6->UseLookupTableScalarRangeOn();

  vtkActor* Actor6 = vtkActor::New();
  Actor6->SetMapper(Mapper6);
  Actor6->GetProperty()->SetRepresentationToSurface();
  Actor6->GetProperty()->SetInterpolationToGouraud();
  Ren1->AddActor(Actor6);

  vtkCompositeRenderManager* compManager = vtkCompositeRenderManager::New();
  compManager->SetRenderWindow(renWin);
  compManager->SetController(contr);
  compManager->InitializePieces();

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  int retVal;

  if (me)
  {
    compManager->InitializeRMIs();
    contr->ProcessRMIs();
    contr->Receive(&retVal, 1, 0, 33);
  }
  else
  {
    renWin->Render();
    retVal = vtkRegressionTester::Test(argc, argv, renWin, 10);
    for (int i = 1; i < numProcs; i++)
    {
      contr->TriggerRMI(i, vtkMultiProcessController::BREAK_RMI_TAG);
      contr->Send(&retVal, 1, i, 33);
    }
  }

  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
  {
    compManager->StartInteractor();
  }

  contr->Finalize();
  contr->Delete();

  iren->Delete();
  renWin->Delete();
  compManager->Delete();
  Actor6->Delete();
  Mapper6->Delete();
  Tuber0->Delete();
  Probe0->Delete();
  probeLine->Delete();
  Actor4->Delete();
  Mapper4->Delete();
  Geometry4->Delete();
  ironProt0->Delete();
  Ren1->Delete();

  return !retVal;
}
