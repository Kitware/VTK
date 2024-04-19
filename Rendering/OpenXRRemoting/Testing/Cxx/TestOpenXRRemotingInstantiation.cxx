// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkOpenXRCamera.h"
#include "vtkOpenXRRemotingRenderWindow.h"
#include "vtkOpenXRRenderWindowInteractor.h"
#include "vtkOpenXRRenderer.h"
#include "vtkPolyDataMapper.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

//------------------------------------------------------------------------------
int TestOpenXRRemotingInstantiation(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkOpenXRRenderer> renderer;
  vtkNew<vtkOpenXRRemotingRenderWindow> renderWindow;
  vtkNew<vtkOpenXRRenderWindowInteractor> iren;
  vtkNew<vtkOpenXRCamera> cam;
  renderWindow->AddRenderer(renderer);
  iren->SetRenderWindow(renderWindow);
  renderer->SetActiveCamera(cam);
  renderWindow->SetRemotingIPAddress("127.0.0.1");

  vtkNew<vtkSphereSource> src;
  src->SetCenter(0, 0, -1);
  src->SetRadius(0.1);
  src->Update();

  vtkNew<vtkPolyDataMapper> srcMapper;
  srcMapper->SetInputData(src->GetOutput());
  srcMapper->Update();

  vtkNew<vtkActor> srcActor;
  srcActor->SetMapper(srcMapper);
  renderer->AddActor(srcActor);

  return EXIT_SUCCESS;
}
