// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenVRCamera.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkOpenVRRenderer.h"

//------------------------------------------------------------------------------
// Only instanciates, do not requires a OpenVR implementation to run
int TestOpenVRInstantiation(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkOpenVRRenderer> renderer;
  vtkNew<vtkOpenVRRenderWindow> renderWindow;
  vtkNew<vtkOpenVRCamera> cam;
  vtkNew<vtkOpenVRRenderWindowInteractor> iren;
  vtkNew<vtkActor> actor;

  renderer->SetActiveCamera(cam);
  renderer->AddActor(actor);
  renderWindow->AddRenderer(renderer);
  iren->SetRenderWindow(renderWindow);
  return 0;
}
