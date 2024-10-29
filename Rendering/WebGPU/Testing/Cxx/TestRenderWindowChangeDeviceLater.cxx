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
  if (auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renWin))
  {
    // When both intel and nvidia are available, vulkan doesn't work with the intel gpu.
    // So far, this is only observed in linux as there is no win32 webgpu render window yet.
    // X Error of failed request:  BadMatch (invalid parameter attributes)
    //   Major opcode of failed request:  149 ()
    //   Minor opcode of failed request:  4
    wgpuConfig->Initialize();
    const auto nvidiaInUseBeforePowerPrefChange = wgpuConfig->IsNVIDIAGPUInUse();
    wgpuConfig->Finalize();

    wgpuConfig->SetPowerPreference(vtkWebGPUConfiguration::PowerPreferenceType::LowPower);
    wgpuConfig->Initialize();

    if (nvidiaInUseBeforePowerPrefChange && wgpuConfig->IsIntelGPUInUse())
    {
      return VTK_SKIP_RETURN_CODE;
    }
    wgpuRenderWindow->SetWGPUConfiguration(wgpuConfig);
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
