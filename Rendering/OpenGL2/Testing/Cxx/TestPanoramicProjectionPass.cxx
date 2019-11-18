/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPanoramicProjectionPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the tone mapping post-processing render pass.
// It renders an opaque actor with a lot of lights.

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkCullerCollection.h"
#include "vtkLight.h"
#include "vtkLightsPass.h"
#include "vtkOpaquePass.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPanoramicProjectionPass.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSequencePass.h"
#include "vtkSphereSource.h"

int TestPanoramicProjectionPass(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(1.0);

  vtkNew<vtkRenderer> renderer;
  renderer->GetCullers()->RemoveAllItems();
  renderer->SetBackground(1.0, 1.0, 1.0);
  renderer->AutomaticLightCreationOff();

  vtkNew<vtkLight> light;
  light->SetPosition(0.0, 10.0, 0.0);
  light->SetFocalPoint(0.0, 0.0, 0.0);
  light->SetLightTypeToSceneLight();
  renderer->AddLight(light);

  // custom passes
  vtkNew<vtkCameraPass> cameraP;
  vtkNew<vtkSequencePass> seq;
  vtkNew<vtkOpaquePass> opaque;
  vtkNew<vtkLightsPass> lights;

  vtkNew<vtkRenderPassCollection> passes;
  passes->AddItem(lights);
  passes->AddItem(opaque);
  seq->SetPasses(passes);
  cameraP->SetDelegatePass(seq);

  vtkNew<vtkPanoramicProjectionPass> projectionP;
  projectionP->SetProjectionTypeToAzimuthal();
  projectionP->SetAngle(360.0);
  projectionP->SetDelegatePass(cameraP);

  vtkOpenGLRenderer::SafeDownCast(renderer)->SetPass(projectionP);

  renWin->AddRenderer(renderer);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  for (int i = 0; i < 4; i++)
  {
    double f = (i & 1) ? -2.0 : 2.0;
    double x = (i & 2) ? 1.0 : 0.0;
    double c[3] = { static_cast<double>((i + 1) & 1), static_cast<double>(((i + 1) >> 1) & 1),
      static_cast<double>(((i + 1) >> 2) & 1) };
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->SetPosition(f * x, 0.0, f * (1.0 - x));
    actor->GetProperty()->SetColor(c);
    renderer->AddActor(actor);
  }

  vtkNew<vtkCamera> camera;
  camera->SetPosition(0.0, 0.0, 0.0);
  camera->SetFocalPoint(0.0, 0.0, 1.0);
  camera->SetViewUp(0.0, 1.0, 0.0);

  renderer->SetActiveCamera(camera);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
