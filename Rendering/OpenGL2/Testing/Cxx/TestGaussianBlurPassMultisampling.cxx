// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This test renders a two-layer render window with multisampling explicitly enabled:
// ctest exports VTK_TESTING, which normally forces multisampling off, so this is the
// only coverage of that combination. The layer 1 renderer carries a render pass, which
// must preserve the layer 0 color buffer by copying it through the render window's
// resolve framebuffer -- a scaled blit straight out of the multisampled framebuffer is
// undefined and fails under GLES. The test passes when the layer 0 sphere survives into
// the final image.

#include "vtkActor.h"
#include "vtkGaussianBlurPass.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderStepsPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkUnsignedCharArray.h"

#include <cstdlib>
#include <iostream>

int TestGaussianBlurPassMultisampling(int, char*[])
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(32);
  sphere->SetPhiResolution(32);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.2);
  renderer->AddActor(actor);

  // A layer > 0 renderer preserves the color buffer of the layers below it.
  vtkNew<vtkRenderer> layerRenderer;
  layerRenderer->SetLayer(1);

  vtkNew<vtkRenderStepsPass> basicPasses;
  vtkNew<vtkGaussianBlurPass> blurPass;
  blurPass->SetDelegatePass(basicPasses);
  layerRenderer->SetPass(blurPass);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->SetNumberOfLayers(2);
  renderWindow->AddRenderer(renderer);
  renderWindow->AddRenderer(layerRenderer);
  renderWindow->SetMultiSamples(8);

  renderWindow->Render();

  vtkOpenGLRenderWindow* glRenderWindow = vtkOpenGLRenderWindow::SafeDownCast(renderWindow);
  if (!glRenderWindow->GetBufferNeedsResolving())
  {
    // The pass still has to preserve the color buffer, so keep checking, but flag that
    // the multisampled path this test exists for was not taken.
    std::cout << "WARNING: context did not provide multisampling, resolve path not covered\n";
  }

  vtkNew<vtkUnsignedCharArray> pixels;
  renderWindow->GetPixelData(0, 0, 299, 299, /* front = */ 0, pixels);

  // The blurred sphere has to dominate the center pixel and the background the corner
  // pixel; if the pass lost the layer 0 color buffer, both come back black.
  unsigned char center[3];
  unsigned char corner[3];
  for (int comp = 0; comp < 3; ++comp)
  {
    center[comp] = pixels->GetTypedComponent(150 * 300 + 150, comp);
    corner[comp] = pixels->GetTypedComponent(10 * 300 + 10, comp);
  }
  if (center[0] < 100 || center[0] < 2 * center[1] || center[0] < 2 * center[2])
  {
    std::cerr << "ERROR: expected a red sphere at the center, got (" << +center[0] << ", "
              << +center[1] << ", " << +center[2] << ")\n";
    return EXIT_FAILURE;
  }
  if (corner[2] < 20 || corner[2] < corner[0])
  {
    std::cerr << "ERROR: expected the blue background in the corner, got (" << +corner[0] << ", "
              << +corner[1] << ", " << +corner[2] << ")\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
