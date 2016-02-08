/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOsprayDynamicScene.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that dynamic scene contents work acceptably
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkOsprayPass.h"
#include "vtkOsprayViewNodeFactory.h"
#include "vtkOsprayWindowNode.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

#include <map>

int TestOsprayDynamicScene(int argc, char* argv[])
{
  int retVal = 1;

  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.0,0.0,0.0);
  renWin->SetSize(400,400);
  renWin->Render();

  vtkSmartPointer<vtkOsprayViewNodeFactory> vnf = vtkSmartPointer<vtkOsprayViewNodeFactory>::New();
  vtkViewNode *vn = vnf->CreateNode(renWin);
  vn->Build();

  vtkSmartPointer<vtkOsprayPass> ospray=vtkSmartPointer<vtkOsprayPass>::New();
  ospray->SetSceneGraph(vtkOsprayWindowNode::SafeDownCast(vn));
  renderer->SetPass(ospray);
  //renWin->Render();

  #define NUMACTS 3
  vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
  camera->SetPosition(NUMACTS*3,NUMACTS*3,NUMACTS*4);
  renderer->SetActiveCamera(camera);

  std::map<int, vtkActor*> actors;
  for (int i = 0; i < NUMACTS; i++)
    {
    for (int j = 0; j < NUMACTS; j++)
      {
      for (int k = 0; k < NUMACTS; k++)
        {
        vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
        sphere->SetCenter(i,j,k);
        sphere->SetPhiResolution(10);
        sphere->SetThetaResolution(10);
        vtkSmartPointer<vtkPolyDataMapper> mapper=vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(sphere->GetOutputPort());
        vtkActor *actor= vtkActor::New();
        renderer->AddActor(actor);
        actor->SetMapper(mapper);
        actors[i*NUMACTS*NUMACTS+j*NUMACTS+k] = actor;
        renWin->Render();
        }
      }
    }

  for (int i = 0; i < NUMACTS; i++)
    {
    for (int j = 0; j < NUMACTS; j++)
      {
      for (int k = 0; k < NUMACTS; k++)
        {
        vtkActor *actor = actors[i*NUMACTS*NUMACTS+j*NUMACTS+k];
        actor->VisibilityOff();
        renWin->Render();
        }
      }
    }

  for (int i = 0; i < NUMACTS; i++)
    {
    for (int j = 0; j < NUMACTS; j++)
      {
      for (int k = 0; k < NUMACTS; k++)
        {
        vtkActor *actor = actors[i*NUMACTS*NUMACTS+j*NUMACTS+k];
        actor->VisibilityOn();
        renWin->Render();
        }
      }
    }

  for (int i = 0; i < NUMACTS; i++)
    {
    for (int j = 0; j < NUMACTS; j++)
      {
      for (int k = 0; k < NUMACTS; k++)
        {
        vtkActor *actor = actors[i*NUMACTS*NUMACTS+j*NUMACTS+k];
        renderer->RemoveActor(actor);
        actor->Delete();
        renWin->Render();
        }
      }
    }


  vn->Delete();

  //iren->Start();


  return !retVal;
}
