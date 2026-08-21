// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// When nothing hands the depth peeling pass an opaque Z texture, it copies the depth
// buffer out of the render framebuffer itself with vtkTextureObject::CopyFromFrameBuffer.
// The other depth peeling tests all attach a vtkFramebufferPass with its own textures
// precisely to avoid that copy (renderer level depth peeling refuses to run at all under
// GLES, so a pass pipeline like this one is the only route there), and this test
// exercises the copy on purpose, across the framebuffer configurations that change it:
// a packed depth-stencil buffer (stencil-capable window) and a multisampled buffer that
// has to be resolved first. ctest exports VTK_TESTING, which defaults MultiSamples to
// zero, so the multisampled configurations exist nowhere else. The checks are
// programmatic so that sample counts cannot perturb an image comparison: a translucent
// sphere behind an opaque one must stay hidden (a broken Z copy reading far-plane depths
// lets it bleed through), and the part of the translucent sphere sticking out past the
// opaque one must stay visible (a broken copy reading near-plane depths discards every
// translucent fragment).

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDepthPeelingPass.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderStepsPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkUnsignedCharArray.h"

#include <cstdlib>
#include <iostream>

namespace
{

bool RunScenario(bool stencilCapable, int multiSamples)
{
  std::cout << "Scenario: stencil " << (stencilCapable ? "on" : "off") << ", " << multiSamples
            << " multisamples\n";

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(32);
  sphere->SetPhiResolution(32);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  // an opaque sphere in front, and a larger translucent sphere centered behind it, so
  // that the translucent one only shows in a ring around the opaque one
  vtkNew<vtkActor> opaqueActor;
  opaqueActor->SetMapper(mapper);
  opaqueActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
  opaqueActor->GetProperty()->LightingOff();

  vtkNew<vtkActor> translucentActor;
  translucentActor->SetMapper(mapper);
  translucentActor->SetPosition(0.0, 0.0, -2.0);
  translucentActor->SetScale(2.0);
  translucentActor->GetProperty()->SetColor(0.0, 0.0, 1.0);
  translucentActor->GetProperty()->SetOpacity(0.5);
  translucentActor->GetProperty()->LightingOff();

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.1, 0.1, 0.1);
  renderer->AddActor(opaqueActor);
  renderer->AddActor(translucentActor);

  // A depth peeling pass that is handed no opaque Z texture, so it copies the depth
  // buffer out of the render framebuffer itself.
  vtkNew<vtkRenderStepsPass> basicPasses;
  vtkNew<vtkDepthPeelingPass> peeling;
  peeling->SetMaximumNumberOfPeels(8);
  peeling->SetOcclusionRatio(0.0);
  peeling->SetTranslucentPass(basicPasses->GetTranslucentPass());
  basicPasses->SetTranslucentPass(peeling);
  vtkOpenGLRenderer::SafeDownCast(renderer)->SetPass(basicPasses);

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->ParallelProjectionOn();
  camera->SetParallelScale(1.5);
  camera->SetPosition(0.0, 0.0, 5.0);
  camera->SetFocalPoint(0.0, 0.0, 0.0);
  camera->SetClippingRange(1.0, 10.0);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->SetAlphaBitPlanes(1);
  renderWindow->SetStencilCapable(stencilCapable ? 1 : 0);
  renderWindow->SetMultiSamples(multiSamples);
  renderWindow->AddRenderer(renderer);

  renderWindow->Render();

  if (multiSamples > 0 &&
    !vtkOpenGLRenderWindow::SafeDownCast(renderWindow)->GetBufferNeedsResolving())
  {
    std::cout << "WARNING: context did not provide multisampling, resolve path not covered\n";
  }

  vtkNew<vtkUnsignedCharArray> pixels;
  renderWindow->GetPixelData(0, 0, 299, 299, /* front = */ 0, pixels);

  // parallel scale 1.5 on a 300 pixel window puts 100 pixels per world unit: the center
  // pixel sees the opaque unit sphere with the translucent one behind it, and 75 pixels
  // to the right only the translucent sphere covers the background
  unsigned char center[3];
  unsigned char ring[3];
  for (int comp = 0; comp < 3; ++comp)
  {
    center[comp] = pixels->GetTypedComponent(150 * 300 + 150, comp);
    ring[comp] = pixels->GetTypedComponent(150 * 300 + 225, comp);
  }
  if (center[0] < 200 || center[2] > 60)
  {
    std::cerr << "ERROR: the translucent sphere bled through the opaque one: center pixel is ("
              << +center[0] << ", " << +center[1] << ", " << +center[2] << ")\n";
    return false;
  }
  if (ring[2] < 100 || ring[0] > 60)
  {
    std::cerr << "ERROR: the translucent sphere is missing next to the opaque one: pixel is ("
              << +ring[0] << ", " << +ring[1] << ", " << +ring[2] << ")\n";
    return false;
  }
  return true;
}

} // anonymous namespace

int TestDepthPeelingOwnOpaqueZTexture(int, char*[])
{
  bool success = true;
  success &= RunScenario(false, 0);
  success &= RunScenario(true, 0);
  success &= RunScenario(false, 4);
  success &= RunScenario(true, 4);
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
