/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayDynamicObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can render dynamic objects (changing mesh)
// and that changing vtk state changes the resulting image accordingly.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

//TODO: test broken by pre SC15 ospray caching

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkOSPRayPass.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

int TestOSPRayDynamicObject(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);
  vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetPhiResolution(100);
  vtkSmartPointer<vtkPolyDataMapper> mapper=vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(sphere->GetOutputPort());
  vtkSmartPointer<vtkActor> actor=vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  renderer->SetBackground(0.1,0.1,1.0);
  renWin->SetSize(400,400);
  renWin->Render();

  vtkSmartPointer<vtkOSPRayPass> ospray=vtkSmartPointer<vtkOSPRayPass>::New();

  renderer->SetPass(ospray);
  renWin->Render();

  vtkLight *light = vtkLight::SafeDownCast(renderer->GetLights()->GetItemAsObject(0));
  double lColor[3];
  lColor[0] = 0.5;
  lColor[1] = 0.5;
  lColor[2] = 0.5;
  light->SetDiffuseColor(lColor[0],lColor[1],lColor[2]);

  vtkCamera *camera = renderer->GetActiveCamera();
  double position[3];
  camera->GetPosition(position);
  camera->SetClippingRange(0.01,1000.0);

#define MAXFRAME 20
  double inc = 1.0/(double)MAXFRAME;

  for (int i = 0; i < MAXFRAME; i++)
  {
    double I = (double)i/(double)MAXFRAME;
    renWin->SetSize(400+i,400-i);
    sphere->SetThetaResolution(3+i);

    lColor[0] += inc/2;
    lColor[1] -= inc/2;
    light->SetDiffuseColor(lColor[0],lColor[1],lColor[2]);

    if (i < (MAXFRAME/2))
    {
      position[2] += inc*5;
    }
    else
    {
      position[2] -= inc*5;
    }

    camera->SetPosition(position);

    renderer->SetBackground(0.0,I,1-I);
    renWin->Render();
  }

  iren->Start();

  return 0;
}
