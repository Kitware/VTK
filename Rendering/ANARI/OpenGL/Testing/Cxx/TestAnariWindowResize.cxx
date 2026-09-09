// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include "vtkAnariOpenGLTestUtilities.h"
#include "vtkAnariPass.h"

/**
 * This test verifies that the window resize re-render the frame correctly.
 */
int TestAnariWindowResize(int argc, char* argv[])
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
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkAnariPass> anariPass;
  renderer->SetPass(anariPass);

  vtkAnariOpenGLTestUtilities::SetParameterDefaults(
    anariPass, renderer, useDebugDevice, "TestAnariWindowResize");

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  renderWindow->SetSize(1280, 720);
  renderWindow->Render();

  renderWindow->SetSize(300, 300);
  int result = vtkRegressionTestImage(renderWindow);

  return result == vtkTesting::PASSED ? EXIT_SUCCESS : EXIT_FAILURE;
}
