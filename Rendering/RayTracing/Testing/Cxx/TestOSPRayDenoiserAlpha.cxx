// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test verifies that a denoised frame keeps its alpha channel. The
// OpenImageDenoise RT filter writes only the RGB channels, so denoising into a
// separately allocated output buffer left the alpha at whatever that buffer
// happened to hold and the frame came back fully transparent.

#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include <cstdlib>
#include <iostream>

int TestOSPRayDenoiserAlpha(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(50);
  sphere->SetThetaResolution(50);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkOSPRayPass> ospray;
  renderer->SetPass(ospray);

  // Denoising only happens once the accumulated samples reach the threshold,
  // and only on the path that reads the frame back rather than handing OpenGL
  // a texture, so ask for both.
  vtkOSPRayRendererNode::SetSamplesPerPixel(4, renderer);
  vtkOSPRayRendererNode::SetDenoiserThreshold(1, renderer);
  vtkOSPRayRendererNode::SetEnableDenoiser(1, renderer);
  vtkOSPRayRendererNode::SetCompositeOnGL(0, renderer);

  renderer->ResetCamera();
  renderWindow->Render();

  vtkOSPRayRendererNode* sceneGraph = ospray->GetSceneGraph();
  if (!sceneGraph)
  {
    std::cerr << "Problem, the OSPRay pass has no scene graph to read back." << std::endl;
    return EXIT_FAILURE;
  }

  const float* rgba = static_cast<const float*>(sceneGraph->GetBuffer());
  if (!rgba)
  {
    std::cerr << "Problem, the OSPRay pass rendered no color buffer." << std::endl;
    return EXIT_FAILURE;
  }

  const int* size = renderer->GetSize();
  const vtkIdType pixels = static_cast<vtkIdType>(size[0]) * static_cast<vtkIdType>(size[1]);
  vtkIdType opaque = 0;
  for (vtkIdType i = 0; i < pixels; ++i)
  {
    if (rgba[4 * i + 3] > 0.5f)
    {
      ++opaque;
    }
  }

  std::cout << opaque << " of " << pixels << " pixels in the denoised frame are opaque."
            << std::endl;

  if (opaque == 0)
  {
    std::cerr << "Problem, the denoised frame is fully transparent. The denoiser filled a "
                 "fresh output buffer whose alpha was never written."
              << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
