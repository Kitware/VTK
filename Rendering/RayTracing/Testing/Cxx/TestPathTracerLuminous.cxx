/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPathTracerLuminous.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that light emitting objects work in ospray's path tracer.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
//              In interactive mode it responds to the keys listed
//              vtkOSPRayTestInteractor.h
//
// "Luminous beings are we, not this crude matter."

#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkOSPRayActorNode.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

#include "vtkOSPRayTestInteractor.h"

int TestPathTracerLuminous(int argc, char* argv[])
{
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetSize(400, 400);
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->AutomaticLightCreationOff();
  renderer->SetBackground(0.0, 0.0, 0.0);
  renderer->UseShadowsOn();
  vtkOSPRayRendererNode::SetSamplesPerPixel(30, renderer);
  renWin->AddRenderer(renderer);

  vtkSmartPointer<vtkCamera> c = vtkSmartPointer<vtkCamera>::New();
  c->SetPosition(0, 0, 80);
  c->SetFocalPoint(0, 0, 0);
  c->SetViewUp(0, 1, 0);
  renderer->SetActiveCamera(c);

  vtkSmartPointer<vtkSphereSource> ss = vtkSmartPointer<vtkSphereSource>::New();
  ss->SetCenter(11, 1, 20);
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(ss->GetOutputPort());
  vtkSmartPointer<vtkActor> actor1 = vtkSmartPointer<vtkActor>::New();
  vtkSmartPointer<vtkProperty> prop = actor1->GetProperty();
  prop->SetColor(1, 1, 0);
  vtkOSPRayActorNode::SetLuminosity(200, prop);
  renderer->AddActor(actor1);
  actor1->SetMapper(mapper);

  vtkSmartPointer<vtkPlaneSource> shadowee = vtkSmartPointer<vtkPlaneSource>::New();
  shadowee->SetOrigin(-10, -10, 0);
  shadowee->SetPoint1(10, -10, 0);
  shadowee->SetPoint2(-10, 10, 0);
  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(shadowee->GetOutputPort());
  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkPlaneSource> shadower = vtkSmartPointer<vtkPlaneSource>::New();
  shadower->SetOrigin(-5, -5, 10);
  shadower->SetPoint1(5, -5, 10);
  shadower->SetPoint2(-5, 5, 10);
  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(shadower->GetOutputPort());
  actor = vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkOSPRayPass> ospray = vtkSmartPointer<vtkOSPRayPass>::New();
  renderer->SetPass(ospray);
  vtkOSPRayRendererNode::SetRendererType("pathtracer", renderer);
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      vtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      break;
    }
  }

  for (double i = 0.; i < 2.0; i += 0.25)
  {
    vtkOSPRayActorNode::SetLuminosity(200 + i * 400, prop);
    renWin->Render();
  }

  vtkSmartPointer<vtkOSPRayTestInteractor> style = vtkSmartPointer<vtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();
  return 0;
}
