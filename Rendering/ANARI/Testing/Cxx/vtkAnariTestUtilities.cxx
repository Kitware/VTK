// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariTestUtilities.h"

#include "vtkAnariPass.h"
#include "vtkAnariRenderWindow.h"
#include "vtkAnariSceneGraph.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"

namespace
{

//------------------------------------------------------------------------------
void SetParameterDefaultsInternal(vtkAnariDevice* device, vtkAnariRenderer* anariRenderer,
  vtkRenderer* nativeRenderer, bool useDebugDevice, const char* testName)
{
  if (useDebugDevice)
  {
    vtkNew<vtkTesting> testing;
    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace/";
    traceDir += testName;
    device->SetAnariDebugConfig(traceDir.c_str(), "code");
  }

  device->SetupAnariDeviceFromLibrary("environment", "default", useDebugDevice);

  // General renderer parameters:
  device->SetParameterf("ambientRadiance", 1.f);

  // VisRTX specific renderer parameters:
  anariRenderer->SetParameterf("lightFalloff", 0.5f);
  anariRenderer->SetParameterb("denoise", true);
  anariRenderer->SetParameteri("pixelSamples", 8);

  if (nativeRenderer)
  {
    vtkAnariSceneGraph::SetCompositeOnGL(nativeRenderer, 1);
  }
}

}

namespace vtkAnariTestUtilities
{

//------------------------------------------------------------------------------
void SetParameterDefaults(
  vtkAnariRenderWindow* renderWindow, bool useDebugDevice, const char* testName)
{
  if (!renderWindow)
  {
    return;
  }

  auto* anariDevice = renderWindow->GetAnariDevice();
  auto* anariRenderer = renderWindow->GetAnariRenderer();

  ::SetParameterDefaultsInternal(anariDevice, anariRenderer, nullptr, useDebugDevice, testName);
}

//------------------------------------------------------------------------------
void SetParameterDefaults(
  vtkAnariPass* pass, vtkRenderer* renderer, bool useDebugDevice, const char* testName)
{
  if (!pass || !renderer)
    return;

  auto* ad = pass->GetAnariDevice();
  auto* ar = pass->GetAnariRenderer();

  ::SetParameterDefaultsInternal(ad, ar, renderer, useDebugDevice, testName);
}

}
