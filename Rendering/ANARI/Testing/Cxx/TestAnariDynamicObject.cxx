// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test verifies that we can render dynamic objects (changing mesh)
// and that changing vtk state changes the resulting image accordingly.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include "vtkAnariPass.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestInteractor.h"
#include "vtkAnariTestUtilities.h"

int TestAnariDynamicObject(int argc, char* argv[])
{
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_WARNING);
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-trace"))
    {
      useDebugDevice = true;
      vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
    }
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);
  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(100);
  sphere->SetThetaResolution(100);
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> actor;
  vtkProperty* prop = actor->GetProperty();
  prop->SetMaterialName("matte");
  prop->SetColor(1.0, 0.0, 0.0); // Red
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  renderer->SetBackground(0.1, 0.1, 1.0);
  renWin->SetSize(400, 400);
  renWin->Render();

  vtkNew<vtkAnariPass> anariPass;
  renderer->SetPass(anariPass);

  SetParameterDefaults(anariPass, renderer, useDebugDevice, "TestAnariDynamicObject");

  renWin->Render();

  vtkLight* light = vtkLight::SafeDownCast(renderer->GetLights()->GetItemAsObject(0));
  double lColor[3];
  lColor[0] = 0.5;
  lColor[1] = 0.5;
  lColor[2] = 0.5;
  light->SetDiffuseColor(lColor[0], lColor[1], lColor[2]);

  vtkCamera* camera = renderer->GetActiveCamera();
  double position[3];
  camera->GetPosition(position);
  camera->SetClippingRange(0.01, 1000.0);

#define MAXFRAME 20
  double inc = 1.0 / (double)MAXFRAME;

  for (int i = 0; i < MAXFRAME; i++)
  {
    double I = (double)i / (double)MAXFRAME;
    renWin->SetSize(400 + i, 400 - i);
    sphere->SetPhiResolution(3 + i);
    sphere->SetThetaResolution(3 + i);

    lColor[0] += inc / 2;
    lColor[1] -= inc / 2;
    light->SetDiffuseColor(lColor[0], lColor[1], lColor[2]);

    if (i < (MAXFRAME / 2))
    {
      position[2] += inc * 5;
    }
    else
    {
      position[2] -= inc * 5;
    }

    camera->SetPosition(position);

    renderer->SetBackground(0.0, I, 1 - I);
    renWin->Render();
  }

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkAnariTestInteractor> style;
    style->SetPipelineControlPoints(renderer, anariPass, nullptr);
    iren->SetInteractorStyle(style);
    style->SetCurrentRenderer(renderer);

    iren->Start();
  }

  return !retVal;
}
