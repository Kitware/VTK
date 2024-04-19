// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkOpenXRCamera.h"
#include "vtkOpenXRManagerRemoteConnection.h"
#include "vtkOpenXRRemotingRenderWindow.h"
#include "vtkOpenXRRenderWindowInteractor.h"
#include "vtkOpenXRRenderer.h"
#include "vtkPolyDataMapper.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

#include "vtksys/SystemTools.hxx"

namespace
{

bool TestOpenXRManagerRemotingConnection()
{
  const std::string dummy{ "dummy.json" };
  vtksys::SystemTools::PutEnv("XR_RUNTIME_JSON=" + dummy);

  vtkNew<vtkOpenXRManagerRemoteConnection> cs;
  cs->Initialize();

  std::string env;
  if (!vtksys::SystemTools::GetEnv("XR_RUNTIME_JSON", env) || env == dummy)
  {
    std::cout << "XR_RUNTIME_JSON must be defined after cs->Initialize()" << std::endl;
    return false;
  }

  cs->EndInitialize();
  if (!vtksys::SystemTools::GetEnv("XR_RUNTIME_JSON", env) || env != dummy)
  {
    std::cout << "XR_RUNTIME_JSON must be restored to its original value after cs->EndInitialize"
              << std::endl;
    return false;
  }

  // let the XR_RUNTIME_JSON environment variable defined so next test will indirectly check
  // that it does not affect OpenXR Remoting initialization.
  return true;
}

}

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

  if (!TestOpenXRManagerRemotingConnection())
  {
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
