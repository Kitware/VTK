/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOsprayDynamicObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can render objects and that changing vtk state
// changes the resulting image accordingly. It
// runs for a while so that we can verify that we have no leaks.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkOsprayPass.h"
#include "vtkOsprayViewNodeFactory.h"
#include "vtkOsprayWindowNode.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

int TestOsprayDynamicObject(int argc, char* argv[])
{
  int retVal = 1;

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

  vtkSmartPointer<vtkOsprayViewNodeFactory> vnf = vtkSmartPointer<vtkOsprayViewNodeFactory>::New();
  vtkViewNode *vn = vnf->CreateNode(renWin);
  vn->Build();

  vtkSmartPointer<vtkOsprayPass> ospray=vtkSmartPointer<vtkOsprayPass>::New();
  ospray->SetSceneGraph(vtkOsprayWindowNode::SafeDownCast(vn));

  renderer->SetPass(ospray);
  renWin->Render();

  vtkLight *light = vtkLight::SafeDownCast(renderer->GetLights()->GetItemAsObject(0));
  double lColor[3];
  light->GetDiffuseColor(lColor);

  vtkCamera *camera = renderer->GetActiveCamera();
  double position[3];
  camera->GetPosition(position);

#define MAXFRAME 100

  for (int i = 0; i < MAXFRAME; i++)
    {
    double I = (double)i/(double)MAXFRAME;

    renWin->SetSize(400+i,400-i); //TODO: not working yet

    sphere->SetThetaResolution(3+i);

    lColor[0] += I;
    lColor[1] -= I;
    light->SetDiffuseColor(lColor[0],lColor[1],lColor[2]); //TODO: not working, nor is lposition

    position[2] += I;
    camera->SetPosition(position);

    renderer->SetBackground(0.0,I,1-I);
    renWin->Render();
    }

  vn->Delete();

  //iren->Start();

  return !retVal;
}
