// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test ensures that the face culling property of a vtkWebGPUActor is taken into account during
 * rendering
 */
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

int TestActorFaceCullingProperty(int argc, char* argv[])
{
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetPhiResolution(10);
  sphereSource->SetThetaResolution(10);
  sphereSource->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(sphereSource->GetOutput());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->FrontfaceCullingOn();

  vtkNew<vtkRenderWindow> renwin;
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.1, 0.2, 0.4);
  renderer->ResetCamera();
  renwin->AddRenderer(renderer);
  renwin->Render();

  int retVal = vtkRegressionTestImageThreshold(renwin, 0.05);

  return !retVal;
}
