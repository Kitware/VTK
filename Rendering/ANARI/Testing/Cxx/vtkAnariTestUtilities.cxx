// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariTestUtilities.h"

#include "vtkTesting.h"

void SetAnariRendererParameterDefaults(vtkAnariRendererManager* manager, vtkRenderer* renderer,
  bool useDebugDevice, const char* testName)
{
  if (!manager || !renderer)
    return;

  if (useDebugDevice)
  {
    vtkNew<vtkTesting> testing;
    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace/";
    traceDir += testName;
    manager->SetAnariDebugConfig(traceDir.c_str(), "code");
  }

  manager->SetupAnariDeviceFromLibrary("environment", "default", useDebugDevice);

  // General renderer parameters:
  manager->SetAnariRendererParameter("ambientRadiance", 1.f);

  // VisRTX specific renderer parameters:
  manager->SetAnariRendererParameter("lightFalloff", 0.5f);
  manager->SetAnariRendererParameter("denoise", true);
  manager->SetAnariRendererParameter("pixelSamples", 8);

  vtkAnariRendererNode::SetCompositeOnGL(renderer, 1);
}
