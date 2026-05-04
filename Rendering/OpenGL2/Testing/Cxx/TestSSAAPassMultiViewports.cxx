// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConeSource.h"
#include "vtkDepthPeelingPass.h"
#include "vtkExtractEdges.h"
#include "vtkFramebufferPass.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderStepsPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSSAAPass.h"
#include "vtkTextureObject.h"

#include <array>
#include <cstdlib>

int TestSSAAPassMultiViewports(int argc, char* argv[])
{
  // Create render window
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 800);
  renderWindow->SetMultiSamples(0);
  renderWindow->SetNumberOfLayers(1);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);
  vtkNew<vtkInteractorStyleRubberBandPick> style;
  interactor->SetInteractorStyle(style);

  // Viewport definitions (xmin, ymin, xmax, ymax)
  std::array<std::array<double, 4>, 4> viewports = { {
    { { 0.0, 0.0, 0.5, 0.5 } }, // Bottom-left
    { { 0.5, 0.0, 1.0, 0.5 } }, // Bottom-right
    { { 0.0, 0.5, 0.5, 1.0 } }, // Top-left
    { { 0.5, 0.5, 1.0, 1.0 } }, // Top-right
  } };

  std::array<std::array<double, 3>, 4> colors = { {
    { { 1, 0, 0 } }, // Red
    { { 0, 1, 0 } }, // Green
    { { 0, 0, 1 } }, // Blue
    { { 1, 1, 0 } }, // Yellow
  } };

  vtkNew<vtkFramebufferPass> fop;
  vtkNew<vtkRenderStepsPass> basicPasses;
  vtkNew<vtkDepthPeelingPass> depthPeelingPass;
  depthPeelingPass->SetMaximumNumberOfPeels(5);
  depthPeelingPass->SetOcclusionRatio(0.0);
  depthPeelingPass->SetTranslucentPass(basicPasses->GetTranslucentPass());
  basicPasses->SetTranslucentPass(depthPeelingPass);

  fop->SetDelegatePass(basicPasses);
  fop->SetDepthFormat(vtkTextureObject::Fixed24);
  depthPeelingPass->SetOpaqueZTexture(fop->GetDepthTexture());
  depthPeelingPass->SetOpaqueRGBATexture(fop->GetColorTexture());
  vtkNew<vtkSSAAPass> ssaaPass;
  ssaaPass->SetDelegatePass(fop);

  vtkNew<vtkConeSource> cone;
  vtkNew<vtkExtractEdges> edges;
  edges->SetInputConnection(cone->GetOutputPort());
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(edges->GetOutputPort(0));
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetLineWidth(4);

  for (std::size_t i = 0; i < viewports.size(); ++i)
  {
    // Create renderer
    vtkNew<vtkRenderer> renderer;
    renderer->AddViewProp(actor);
    renderer->SetViewport(viewports[i][0], viewports[i][1], viewports[i][2], viewports[i][3]);
    renderer->SetBackground(colors[i][0], colors[i][1], colors[i][2]);
    renderer->SetPass(ssaaPass);
    renderWindow->AddRenderer(renderer);
  }

  renderWindow->Render();

  const int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkTesting::DO_INTERACTOR)
  {
    interactor->Start();
  }

  return retVal == vtkTesting::FAILED ? EXIT_FAILURE : EXIT_SUCCESS;
}
