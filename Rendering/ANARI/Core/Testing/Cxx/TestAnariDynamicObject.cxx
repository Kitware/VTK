// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"

/**
 * This test verifies that we can render dynamic objects (changing mesh)
 * and that changing vtk state changes the resulting image accordingly.
 */
int TestAnariDynamicObject(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  vtkNew<vtkSphereSource> sphere;

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> actor;
  vtkProperty* prop = actor->GetProperty();
  prop->SetMaterialName("matte");
  prop->SetColor(1.0, 0.0, 0.0);
  actor->SetMapper(mapper);

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  iren->SetRenderWindow(renWin);

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.1, 0.1, 1.0);
  renderer->AddActor(actor);
  renderer->ResetCamera();
  renWin->AddRenderer(renderer);

  vtkAnariTestUtilities::SetParameterDefaults(renWin, useDebugDevice, "TestAnariDynamicObject");

  // Copy is necessary to manipulate coordinates and make sure SetPosition modifies the camera
  // accordingly.
  double cameraPosition[3];
  renderer->GetActiveCamera()->GetPosition(cameraPosition);

  sphere->SetPhiResolution(5);
  sphere->SetThetaResolution(5);
  cameraPosition[0] += 0.5;
  renderer->SetBackground(0.0, 0.8, 0.2);
  renderer->GetActiveCamera()->SetPosition(cameraPosition);
  renWin->Render();

  sphere->SetPhiResolution(20);
  sphere->SetThetaResolution(20);
  cameraPosition[0] -= 2.0;
  renderer->GetActiveCamera()->SetPosition(cameraPosition);
  renderer->SetBackground(0.0, 0.2, 0.8);
  renWin->Render();

  int result = vtkRegressionTestImage(renWin);
  return result == vtkTesting::PASSED ? EXIT_SUCCESS : EXIT_FAILURE;
}
