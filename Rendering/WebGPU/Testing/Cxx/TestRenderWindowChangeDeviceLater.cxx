// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRegularPolygonSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderWindow.h"

int TestRenderWindowChangeDeviceLater(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);
  renWin->DebugOn();

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRegularPolygonSource> polygon;
  polygon->SetNumberOfSides(3);
  polygon->GeneratePolylineOff();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(polygon->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  renderer->ResetCamera();
  renWin->Render();

  vtkNew<vtkWebGPUConfiguration> wgpuConfig;
  wgpuConfig->DebugOn();
  wgpuConfig->SetPowerPreference(vtkWebGPUConfiguration::PowerPreferenceType::LowPower);
  if (auto* wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(renWin))
  {
    wgpuRenWin->SetWGPUConfiguration(wgpuConfig);
  }
  else
  {
    std::cerr << "This test requires the webgpu object factories\n";
    return EXIT_FAILURE;
  }
  renWin->Render();
  const int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
