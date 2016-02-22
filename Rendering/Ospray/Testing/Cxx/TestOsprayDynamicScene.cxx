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
// This test verifies that dynamic scene (vary number of objects)
// contents work acceptably
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

//TODO: test broken by pre SC15 ospray caching

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

  #define GRIDDIM 3
  vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
  camera->SetPosition(GRIDDIM*3,GRIDDIM*3,GRIDDIM*4);
  renderer->SetActiveCamera(camera);

  std::map<int, vtkActor*> actors;
  for (int i = 0; i < GRIDDIM; i++)
    {
    for (int j = 0; j < GRIDDIM; j++)
      {
      for (int k = 0; k < GRIDDIM; k++)
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
        actors[i*GRIDDIM*GRIDDIM+j*GRIDDIM+k] = actor;
        renWin->Render();
        }
      }
    }

  for (int i = 0; i < GRIDDIM; i++)
    {
    for (int j = 0; j < GRIDDIM; j++)
      {
      for (int k = 0; k < GRIDDIM; k++)
        {
        vtkActor *actor = actors[i*GRIDDIM*GRIDDIM+j*GRIDDIM+k];
        actor->VisibilityOff();
        renWin->Render();
        }
      }
    }

  for (int i = 0; i < GRIDDIM; i++)
    {
    for (int j = 0; j < GRIDDIM; j++)
      {
      for (int k = 0; k < GRIDDIM; k++)
        {
        vtkActor *actor = actors[i*GRIDDIM*GRIDDIM+j*GRIDDIM+k];
        actor->VisibilityOn();
        renWin->Render();
        }
      }
    }

  for (int i = 0; i < GRIDDIM; i++)
    {
    for (int j = 0; j < GRIDDIM; j++)
      {
      for (int k = 0; k < GRIDDIM; k++)
        {
        vtkActor *actor = actors[i*GRIDDIM*GRIDDIM+j*GRIDDIM+k];
        //leaving one to have a decent image to compare against
        bool killme = !(i==0 && j==1 && k==0);
        if (killme)
          {
          renderer->RemoveActor(actor);
          actor->Delete();
          renWin->Render();
          }
        }
      }
    }

  vn->Delete();

  iren->Start();

  renderer->RemoveActor(actors[0*GRIDDIM*GRIDDIM+1*GRIDDIM+0]);
  actors[0*GRIDDIM*GRIDDIM+1*GRIDDIM+0]->Delete();

  return 0;
}
