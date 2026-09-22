// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTypeUInt32Array.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include <cstdlib>

namespace
{
vtkSmartPointer<vtkActor> MakeSphere(double z, const double rgb[3], double opacity)
{
  vtkNew<vtkSphereSource> source;
  source->SetCenter(0.0, 0.0, z);
  source->SetRadius(1.0);
  source->SetThetaResolution(32);
  source->SetPhiResolution(32);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(rgb[0], rgb[1], rgb[2]);
  actor->GetProperty()->SetOpacity(opacity);
  return actor;
}

bool ReadCentreIds(
  vtkWebGPURenderWindow* window, int cx, int cy, vtkTypeUInt32& cellId, vtkTypeUInt32& propId)
{
  vtkNew<vtkTypeUInt32Array> ids;
  window->GetIdsData(cx, cy, cx, cy, ids);
  if (ids->GetNumberOfTuples() < 1 || ids->GetNumberOfComponents() < 2)
  {
    vtkLog(ERROR, "no ids were read back at (" << cx << ", " << cy << ")");
    return false;
  }
  cellId = ids->GetTypedComponent(0, 0);
  propId = ids->GetTypedComponent(0, 1);
  return true;
}
}

int TestDeepFramebufferPicking(int, char*[])
{
  const double red[3] = { 1.0, 0.0, 0.0 };
  const double green[3] = { 0.0, 1.0, 0.0 };
  const double blue[3] = { 0.0, 0.0, 1.0 };

  vtkSmartPointer<vtkActor> opaqueFar = MakeSphere(-2.0, blue, 1.0);
  vtkSmartPointer<vtkActor> translucentMid = MakeSphere(0.0, green, 0.4);
  vtkSmartPointer<vtkActor> translucentNear = MakeSphere(2.0, red, 0.4);

  vtkNew<vtkRenderer> renderer;
  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
  if (wgpuRenderer == nullptr)
  {
    vtkLog(ERROR, "renderer is not a vtkWebGPURenderer");
    return EXIT_FAILURE;
  }
  wgpuRenderer->UseDeepFramebufferTransparencyOn();

  renderer->AddActor(opaqueFar);
  renderer->AddActor(translucentMid);
  renderer->AddActor(translucentNear);
  renderer->SetBackground(0.1, 0.1, 0.1);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(300, 300);
  window->AddRenderer(renderer);
  auto* wgpuWindow = vtkWebGPURenderWindow::SafeDownCast(window);
  if (wgpuWindow == nullptr)
  {
    vtkLog(ERROR, "render window is not a vtkWebGPURenderWindow");
    return EXIT_FAILURE;
  }

  renderer->ResetCamera();
  window->Render();

  const int cx = 150;
  const int cy = 150;
  vtkTypeUInt32 nearCell = 0;
  vtkTypeUInt32 nearProp = 0;
  if (!ReadCentreIds(wgpuWindow, cx, cy, nearCell, nearProp))
  {
    return EXIT_FAILURE;
  }
  if (nearProp == 0)
  {
    vtkLog(ERROR,
      "no prop id at the centre; the translucent fragments were discarded "
      "without their ids reaching the attachment");
    return EXIT_FAILURE;
  }
  if (nearCell == 0)
  {
    vtkLog(ERROR,
      "prop id " << nearProp
                 << " reached the attachment but the cell id did not; cell picking through "
                    "translucent layers will not work");
    return EXIT_FAILURE;
  }

  renderer->RemoveActor(translucentNear);
  window->Render();

  vtkTypeUInt32 midCell = 0;
  vtkTypeUInt32 midProp = 0;
  if (!ReadCentreIds(wgpuWindow, cx, cy, midCell, midProp))
  {
    return EXIT_FAILURE;
  }
  if (midProp == 0)
  {
    vtkLog(ERROR, "no prop id at the centre after removing the nearest sphere");
    return EXIT_FAILURE;
  }
  if (midProp == nearProp)
  {
    vtkLog(ERROR,
      "the centre still reports prop " << midProp
                                       << " after the nearest sphere was removed, so the ids are "
                                          "coming from some layer other than the nearest one");
    return EXIT_FAILURE;
  }

  vtkLog(INFO,
    "nearest layer picked: prop " << nearProp << " cell " << nearCell
                                  << "; after removing it, prop " << midProp << " cell "
                                  << midCell);
  return EXIT_SUCCESS;
}
