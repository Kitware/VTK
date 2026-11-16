// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Checks that a renderer with a tone mapping pass and the occlusion culler draws its frame. The
// culler makes the renderer record the frame once to fill the depth buffer, and the renderer then
// records it again with the passes it configured the first time. A clamp tone mapping does not
// change colors within [0, 1], so the image must match the one rendered without the pass.

#include "vtkActor.h"
#include "vtkCameraPass.h"
#include "vtkCullerCollection.h"
#include "vtkLightsPass.h"
#include "vtkNew.h"
#include "vtkOpaquePass.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkSphereSource.h"
#include "vtkToneMappingPass.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGPUComputeOcclusionCuller.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
std::vector<unsigned char> Capture(vtkRenderWindow* window)
{
  // The culler only records the frame first when props were visible in the previous one.
  window->Render();
  window->Render();
  const int* size = window->GetSize();
  vtkNew<vtkUnsignedCharArray> pixels;
  window->GetRGBACharPixelData(0, 0, size[0] - 1, size[1] - 1, 0, pixels);
  return std::vector<unsigned char>(
    pixels->GetPointer(0), pixels->GetPointer(0) + pixels->GetNumberOfValues());
}
}

int TestComputeOcclusionCullingToneMapping(int, char*[])
{
  vtkNew<vtkRenderWindow> window;
  window->SetMultiSamples(0);
  window->SetSize(300, 300);
  // The occlusion culler uses the resources of the render window.
  window->Initialize();

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.7, 0.4);

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.2, 0.3, 0.4);
  renderer->AddActor(actor);
  renderer->ResetCamera();
  renderer->GetCullers()->RemoveAllItems();
  vtkNew<vtkWebGPUComputeOcclusionCuller> culler;
  culler->SetRenderWindow(vtkWebGPURenderWindow::SafeDownCast(window));
  renderer->GetCullers()->AddItem(culler);
  // Render bundles would replay every prop and leave nothing for the culler to do.
  vtkWebGPURenderer::SafeDownCast(renderer)->UseRenderBundlesOff();
  window->AddRenderer(renderer);

  const std::vector<unsigned char> reference = Capture(window);

  vtkNew<vtkLightsPass> lights;
  vtkNew<vtkOpaquePass> opaque;
  vtkNew<vtkRenderPassCollection> passes;
  passes->AddItem(lights);
  passes->AddItem(opaque);
  vtkNew<vtkSequencePass> sequence;
  sequence->SetPasses(passes);
  vtkNew<vtkCameraPass> camera;
  camera->SetDelegatePass(sequence);
  vtkNew<vtkToneMappingPass> toneMapping;
  toneMapping->SetToneMappingType(vtkToneMappingPass::Clamp);
  toneMapping->SetDelegatePass(camera);
  renderer->SetPass(toneMapping);
  const std::vector<unsigned char> toneMapped = Capture(window);

  int difference = 0;
  for (std::size_t i = 0; i < reference.size(); ++i)
  {
    if (i % 4 != 3)
    {
      difference = std::max(difference, std::abs(int(reference[i]) - int(toneMapped[i])));
    }
  }

  toneMapping->ReleaseGraphicsResources(window);
  renderer->SetPass(nullptr);
  if (difference > 2)
  {
    std::cerr << "With the tone mapping pass, the image differs by " << difference << " levels."
              << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
