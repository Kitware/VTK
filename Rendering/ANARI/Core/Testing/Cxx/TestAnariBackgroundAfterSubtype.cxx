// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include "vtkAnariRenderWindow.h"
#include "vtkAnariRenderer.h"
#include "vtkAnariTestUtilities.h"

#include <cstdlib>
#include <cstring>

/**
 * Changing the renderer subtype makes vtkAnariRenderer release its
 * anari::Renderer and create a new one. This test verifies that renderer
 * parameters set afterwards - the background in particular - still reach the
 * renderer the frame is rendered with, instead of the released handle.
 */
int TestAnariBackgroundAfterSubtype(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);
  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);
  renderWindow->SetSize(100, 100);

  vtkAnariTestUtilities::SetParameterDefaults(
    renderWindow, useDebugDevice, "TestAnariBackgroundAfterSubtype");

  renderer->SetBackground(1.0, 0.0, 0.0);
  renderWindow->Render();

  vtkAnariRenderer* anariRenderer =
    vtkAnariRenderWindow::SafeDownCast(renderWindow)->GetAnariRenderer();

  // VisRTX provides a "quality" renderer. Helide only advertises "default", but
  // ignores the subtype name and creates a fresh renderer for "quality" too.
  // Both devices therefore exercise renderer replacement and verify that the
  // background reaches the new renderer used by the frame.
  const anari::Renderer before = anariRenderer->GetHandle();
  anariRenderer->SetSubtype("quality");
  if (anariRenderer->GetHandle() == before)
  {
    vtkLogF(ERROR, "The renderer subtype change did not create a new anari::Renderer.");
    return EXIT_FAILURE;
  }

  // The new renderer must pick up a background set after the subtype changed.
  renderer->SetBackground(0.0, 1.0, 0.0);
  renderWindow->Render();

  // ... and so must a gradient background.
  renderer->SetBackground(1.0, 0.0, 0.0);
  renderer->SetBackground2(0.0, 0.0, 1.0);
  renderer->GradientBackgroundOn();
  renderWindow->Render();

  int result = vtkRegressionTestImage(renderWindow);
  if (result == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  return !result;
}
