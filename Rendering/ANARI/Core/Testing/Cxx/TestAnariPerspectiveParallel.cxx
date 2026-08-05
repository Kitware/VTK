// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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

#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"

/**
 * This test covers switch from perspective to parallel projection.
 * This test renders a cube with a 45 degree camera angle on the pitch and yaw. With this view
 * angle, we can easily check if the parallel projection works correctly.
 */
int TestAnariPerspectiveParallel(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
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

  renderer->GetActiveCamera()->ParallelProjectionOn();
  renderer->GetActiveCamera()->Pitch(45.0);
  renderer->GetActiveCamera()->Yaw(45.0);
  renderer->ResetCamera();
  renderWindow->Render();

  vtkAnariTestUtilities::SetParameterDefaults(
    renderWindow, useDebugDevice, "TestAnariPerspectiveParallel");

  const auto& extensions = vtkAnariTestUtilities::GetDeviceExtensions(renderWindow);
  if (extensions.ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR)
  {
    int retVal = vtkRegressionTestImageThreshold(renderWindow, 0.05);

    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      interactor->Start();
    }

    return !retVal;
  }

  vtkLogF(WARNING, "Required feature KHR_VOLUME_SCIVIS not supported.");
  return VTK_SKIP_RETURN_CODE;
}
