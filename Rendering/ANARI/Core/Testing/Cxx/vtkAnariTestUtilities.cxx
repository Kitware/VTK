// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariTestUtilities.h"

#include "vtkLogger.h"
#include "vtkRenderWindow.h"
#include "vtkTesting.h"

#include "vtkAnariDevice.h"
#include "vtkAnariRenderWindow.h"
#include "vtkAnariRenderer.h"

namespace vtkAnariTestUtilities
{

void SetParameterDefaults(vtkRenderWindow* renderWindow, bool useDebugDevice, const char* testName)
{
  if (!renderWindow->IsA("vtkAnariRenderWindow"))
  {
    vtkLogF(ERROR, "Expected vtkAnariRenderWindow but got %s", renderWindow->GetClassName());
    return;
  }

  vtkAnariRenderWindow* anariRenderWindow = vtkAnariRenderWindow::SafeDownCast(renderWindow);

  auto* anariDevice = anariRenderWindow->GetAnariDevice();
  auto* anariRenderer = anariRenderWindow->GetAnariRenderer();

  anariDevice->SetupAnariDeviceFromLibrary("environment", "default", useDebugDevice);

  if (useDebugDevice)
  {
    vtkNew<vtkTesting> testing;
    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace/";
    traceDir += testName;
    anariDevice->SetAnariDebugConfig(traceDir.c_str(), "code");
  }

  anariRenderWindow->SetUseDebugDevice(useDebugDevice);

  // General renderer parameters:
  anariRenderer->SetParameterf("ambientRadiance", 1.f);

  // VisRTX specific renderer parameters:
  anariRenderer->SetParameterf("lightFalloff", 0.5f);
  anariRenderer->SetParameterb("denoise", true);
  anariRenderer->SetParameteri("pixelSamples", 8);
}

const anari::Extensions& GetDeviceExtensions(vtkRenderWindow* renderWindow)
{
  vtkAnariRenderWindow* anariRenderWindow = vtkAnariRenderWindow::SafeDownCast(renderWindow);
  if (!anariRenderWindow)
  {
    vtkLogF(ERROR, "Expected vtkAnariRenderWindow but got %s", renderWindow->GetClassName());
  }

  return anariRenderWindow->GetAnariDevice()->GetExtensions();
}

}
