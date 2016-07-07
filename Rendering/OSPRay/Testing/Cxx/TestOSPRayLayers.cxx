/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayLayers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can have multiple render layers
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkOSPRayPass.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

int TestOSPRayLayers(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  renWin->SetNumberOfLayers(2);

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);
  vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetPhiResolution(10);
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(sphere->GetOutputPort());
  vtkSmartPointer<vtkActor> actor=vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  renderer->SetBackground(0.5,0.5,1.0); //should see a light blue background

  vtkSmartPointer<vtkRenderer> renderer2 = vtkSmartPointer<vtkRenderer>::New();
  renderer2->SetLayer(1);
  renWin->AddRenderer(renderer2);
  renderer2->SetBackground(1.0,0.0,0.0); //should not see red background
  vtkSmartPointer<vtkConeSource> cone = vtkSmartPointer<vtkConeSource>::New();
  vtkSmartPointer<vtkPolyDataMapper> mapper2 = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper2->SetInputConnection(cone->GetOutputPort());
  vtkSmartPointer<vtkActor> actor2 = vtkSmartPointer<vtkActor>::New();
  renderer2->AddActor(actor2);
  actor2->SetMapper(mapper2);

  renWin->SetSize(400,400);
  renWin->Render();

  vtkSmartPointer<vtkOSPRayPass> ospray=vtkSmartPointer<vtkOSPRayPass>::New();
  vtkSmartPointer<vtkOSPRayPass> ospray2=vtkSmartPointer<vtkOSPRayPass>::New();

  renderer->SetPass(ospray);
  renderer2->SetPass(ospray2);
  renWin->Render();

  iren->Start();

  return 0;
}
