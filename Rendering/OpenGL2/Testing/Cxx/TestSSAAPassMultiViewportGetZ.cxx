// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkDepthPeelingPass.h"
#include "vtkFramebufferPass.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderStepsPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSSAAPass.h"
#include "vtkSmartPointer.h"
#include "vtkTextureObject.h"

#include <iostream>

namespace
{

vtkSmartPointer<vtkPolyData> MakeQuad(double z)
{
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(-1.0, -1.0, z);
  points->InsertNextPoint(1.0, -1.0, z);
  points->InsertNextPoint(1.0, 1.0, z);
  points->InsertNextPoint(-1.0, 1.0, z);

  vtkIdType ptIds[4] = { 0, 1, 2, 3 };
  vtkNew<vtkCellArray> cells;
  cells->InsertNextCell(4, ptIds);

  vtkNew<vtkPolyData> pd;
  pd->SetPoints(points);
  pd->SetPolys(cells);
  return pd;
}

vtkSmartPointer<vtkRenderPass> CreateRenderPass()
{
  vtkNew<vtkRenderStepsPass> basicPasses;
  vtkNew<vtkDepthPeelingPass> depthPeelingPass;
  depthPeelingPass->SetMaximumNumberOfPeels(5);
  depthPeelingPass->SetOcclusionRatio(0.0);
  depthPeelingPass->SetTranslucentPass(basicPasses->GetTranslucentPass());
  basicPasses->SetTranslucentPass(depthPeelingPass);

  vtkNew<vtkFramebufferPass> fop;
  fop->SetDelegatePass(basicPasses);
  fop->SetDepthFormat(vtkTextureObject::Fixed24);
  depthPeelingPass->SetOpaqueZTexture(fop->GetDepthTexture());
  depthPeelingPass->SetOpaqueRGBATexture(fop->GetColorTexture());

  vtkNew<vtkSSAAPass> ssaaPass;
  ssaaPass->SetDelegatePass(fop);
  return ssaaPass;
}

} // namespace

int TestSSAAPassMultiViewportGetZ(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  const int winSize = 600;

  // 2x2 grid: bottom-left, bottom-right, top-left, top-right
  const double xmins[4] = { 0.0, 0.5, 0.0, 0.5 };
  const double ymins[4] = { 0.0, 0.0, 0.5, 0.5 };
  const double xmaxs[4] = { 0.5, 1.0, 0.5, 1.0 };
  const double ymaxs[4] = { 0.5, 0.5, 1.0, 1.0 };

  // red, blue, green, yellow
  const double bgColors[4][3] = {
    { 1.0, 0.0, 0.0 },
    { 0.0, 0.0, 1.0 },
    { 0.0, 1.0, 0.0 },
    { 1.0, 1.0, 0.0 },
  };

  const double zValues[4] = { 1.0, 2.0, 3.0, 4.0 };

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(winSize, winSize);
  renderWindow->SetMultiSamples(0);
  // renderWindow->OffScreenRenderingOn();

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  vtkNew<vtkRenderer> renderers[4];

  for (int i = 0; i < 4; ++i)
  {
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(MakeQuad(zValues[i]));

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    vtkRenderer* ren = renderers[i].Get();
    ren->SetBackground(bgColors[i][0], bgColors[i][1], bgColors[i][2]);
    ren->SetViewport(xmins[i], ymins[i], xmaxs[i], ymaxs[i]);
    ren->AddActor(actor);
    ren->SetPass(CreateRenderPass());
    ren->ResetCamera(-2.0, 2.0, -2.0, 2.0, 0.0, 4.0);
    renderWindow->AddRenderer(ren);
  }

  renderWindow->Render();
  std::cout << "window-class-name: " << renderWindow->GetClassName() << "\n";
  bool success = true;
  // Query z at the pixel midpoint of each viewport
  for (int i = 0; i < 4; ++i)
  {
    const int mx = static_cast<int>((xmins[i] + xmaxs[i]) / 2.0 * winSize);
    const int my = static_cast<int>((ymins[i] + ymaxs[i]) / 2.0 * winSize);

    const double rendererZ = renderers[i]->GetZ(mx, my);
    const float windowZ = renderWindow->GetZbufferDataAtPoint(mx, my);

    double wpt[4];

    renderers[i]->SetDisplayPoint(static_cast<double>(mx), static_cast<double>(my), rendererZ);
    renderers[i]->DisplayToWorld();
    renderers[i]->GetWorldPoint(wpt);
    const double rendererWorldZ = wpt[2] / wpt[3];

    renderers[i]->SetDisplayPoint(
      static_cast<double>(mx), static_cast<double>(my), static_cast<double>(windowZ));
    renderers[i]->DisplayToWorld();
    renderers[i]->GetWorldPoint(wpt);
    const double windowWorldZ = wpt[2] / wpt[3];
    std::cout << "viewport:" << i << "\n";
    std::cout << " vtkRenderer::GetZ - ";
    if (std::abs(rendererWorldZ - zValues[i]) > 1e-6)
    {
      success &= false;
      std::cout << "FAIL\n";
      std::cout << " expected world z=" << zValues[i] << "  midpoint=(" << mx << "," << my << ")"
                << "  GetZ=" << rendererZ << " (world=" << rendererWorldZ << ")\n";
    }
    else
    {
      std::cout << "PASS\n";
    }
    std::cout << " vtkRenderWindow::GetZbufferDataAtPoint - ";
    if (std::abs(windowWorldZ - zValues[i]) > 1e-6)
    {
      success &= false;
      std::cout << "FAIL\n";
      std::cout << " expected world z=" << zValues[i] << "  midpoint=(" << mx << "," << my << ")"
                << "  GetZbufferDataAtPoint=" << windowZ << " (world=" << windowWorldZ << ")\n";
    }
    else
    {
      std::cout << "PASS\n";
    }
  }

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
