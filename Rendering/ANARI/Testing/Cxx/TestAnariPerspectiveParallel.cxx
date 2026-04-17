// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * This test covers switch from perspective to parallel projection.
 * This test renders a cube with a 45 degree camera angle on the pitch and yaw. With this view
 * angle, we can easily check if the parallel projection works correctly.
 */

#include "vtkCamera.h"
#include "vtkCubeSource.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include "vtkAnariPass.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"

#include <iostream>

int TestAnariPerspectiveParallel(int argc, char* argv[])
{
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_WARNING);
  std::cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << std::endl;
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-trace"))
    {
      useDebugDevice = true;
      vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
    }
  }

  vtkNew<vtkCubeSource> source;

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.1, 0.4, 0.2);
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  // Attach ANARI render pass
  vtkNew<vtkAnariPass> anariPass;
  renderer->SetPass(anariPass);

  renderer->GetActiveCamera()->ParallelProjectionOn();
  renderer->GetActiveCamera()->Pitch(45.0);
  renderer->GetActiveCamera()->Yaw(45.0);
  renderer->ResetCamera();
  renderWindow->Render();

  SetParameterDefaults(anariPass, renderer, useDebugDevice, "TestAnariPerspectiveParallel");

  auto anariRendererNode = anariPass->GetSceneGraph();
  const auto& extensions = anariRendererNode->GetAnariDeviceExtensions();
  if (extensions.ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR)
  {
    int retVal = vtkRegressionTestImageThreshold(renderWindow, 0.05);

    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      interactor->Start();
    }

    return !retVal;
  }

  std::cout << "Required feature KHR_VOLUME_SCIVIS not supported." << std::endl;
  return VTK_SKIP_RETURN_CODE;
}
