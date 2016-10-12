/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayDynamicScene.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that time varying data works as expected in ospray
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

//TODO: test broken by pre SC15 ospray caching

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkInformation.h"
#include "vtkOSPRayPass.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimeSourceExample.h"

int TestOSPRayTime(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.0,0.0,0.0);
  renWin->SetSize(400,400);
  renWin->Render();

  vtkSmartPointer<vtkOSPRayPass> ospray=vtkSmartPointer<vtkOSPRayPass>::New();
  renderer->SetPass(ospray);

  vtkSmartPointer<vtkTimeSourceExample> timeywimey = vtkSmartPointer<vtkTimeSourceExample>::New();
  timeywimey->GrowingOn();
  vtkSmartPointer<vtkDataSetSurfaceFilter> dsf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  dsf->SetInputConnection(timeywimey->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> mapper=vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(dsf->GetOutputPort());
  vtkSmartPointer<vtkActor> actor= vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  renWin->Render();
  renderer->ResetCamera();
  double pos[3];
  vtkCamera *camera = renderer->GetActiveCamera();
  camera->SetFocalPoint(0.0,2.5,0.0);
  camera->GetPosition(pos);
  pos[0]= pos[0] + 6;
  pos[1]= pos[1] + 6;
  pos[2]= pos[2] + 6;
  camera->SetPosition(pos);
  renderer->ResetCameraClippingRange();
  renWin->Render();

  for (int i = 0; i < 20; i++)
  {
    double updateTime = (double)(i%10)/10.0;
    cerr << "t=" << updateTime << endl;
    renderer->SetActiveCamera(camera);
    vtkInformation* outInfo = dsf->GetExecutive()->GetOutputInformation(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), updateTime);
    renderer->ResetCameraClippingRange();
    renWin->Render();
  }
  iren->Start();

  return 0;
}
