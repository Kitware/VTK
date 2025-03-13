// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariTestUtilities.h"

#include "vtkAnariSceneGraph.h"
#include "vtkTesting.h"

void SetParameterDefaults(
  vtkAnariPass* pass, vtkRenderer* renderer, bool useDebugDevice, const char* testName)
{
  if (!pass || !renderer)
    return;

  auto* ad = pass->GetAnariDevice();
  auto* ar = pass->GetAnariRenderer();

  if (useDebugDevice)
  {
    vtkNew<vtkTesting> testing;
    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace/";
    traceDir += testName;
    ad->SetAnariDebugConfig(traceDir.c_str(), "code");
  }

  ad->SetupAnariDeviceFromLibrary("environment", "default", useDebugDevice);

  // General renderer parameters:
  ar->SetParameterf("ambientRadiance", 1.f);

  // VisRTX specific renderer parameters:
  ar->SetParameterf("lightFalloff", 0.5f);
  ar->SetParameterb("denoise", true);
  ar->SetParameteri("pixelSamples", 8);

  vtkAnariSceneGraph::SetCompositeOnGL(renderer, 1);
}
