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
int TestOpenXRRemotingInitialization(int argc, char* argv[])
{
  // This test requires the IP address of the player application to be specified.
  char* playerIP =
    vtkTestUtilities::GetArgOrEnvOrDefault("-playerIP", argc, argv, "VTK_PLAYER_IP", nullptr);

  if (playerIP == nullptr)
  {
    std::cerr << "Usage: The IP address of the player must be specified with \"-playerIP\"."
              << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkOpenXRRenderer> renderer;
  vtkNew<vtkOpenXRRemotingRenderWindow> renderWindow;
  vtkNew<vtkOpenXRRenderWindowInteractor> iren;
  vtkNew<vtkOpenXRCamera> cam;
  renderWindow->AddRenderer(renderer);
  iren->SetRenderWindow(renderWindow);
  renderer->SetActiveCamera(cam);
  renderWindow->SetRemotingIPAddress(playerIP);

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

  iren->Start();

  return EXIT_SUCCESS;
}
