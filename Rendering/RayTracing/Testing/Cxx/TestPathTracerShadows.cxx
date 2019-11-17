/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPathTracerShadows.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that soft shadows work with ospray's path tracer.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
//              In interactive mode it responds to the keys listed
//              vtkOSPRayTestInteractor.h

#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkOSPRayLightNode.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

#include "vtkOSPRayTestInteractor.h"

int TestPathTracerShadows(int argc, char* argv[])
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
  vtkOSPRayRendererNode::SetSamplesPerPixel(50, renderer);
  renWin->AddRenderer(renderer);

  vtkSmartPointer<vtkCamera> c = vtkSmartPointer<vtkCamera>::New();
  c->SetPosition(0, 0, 80);
  c->SetFocalPoint(0, 0, 0);
  c->SetViewUp(0, 1, 0);
  renderer->SetActiveCamera(c);

  vtkSmartPointer<vtkLight> l = vtkSmartPointer<vtkLight>::New();
  l->PositionalOn();
  l->SetPosition(4, 8, 20);
  l->SetFocalPoint(0, 0, 0);
  l->SetLightTypeToSceneLight();
  l->SetIntensity(200.0);
  renderer->AddLight(l);

  vtkSmartPointer<vtkPlaneSource> shadowee = vtkSmartPointer<vtkPlaneSource>::New();
  shadowee->SetOrigin(-10, -10, 0);
  shadowee->SetPoint1(10, -10, 0);
  shadowee->SetPoint2(-10, 10, 0);
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
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
    vtkOSPRayLightNode::SetRadius(i, l);
    renWin->Render();
  }

  vtkSmartPointer<vtkOSPRayTestInteractor> style = vtkSmartPointer<vtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();
  return 0;
}
