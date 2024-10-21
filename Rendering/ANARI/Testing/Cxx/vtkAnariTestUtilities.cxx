// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariTestUtilities.h"

#include "vtkAnariRendererNode.h"
#include "vtkTesting.h"

void SetAnariRendererParameterDefaults(
  vtkAnariPass* pass, vtkRenderer* renderer, bool useDebugDevice, const char* testName)
{
  if (!pass || !renderer)
    return;

  auto& dm = pass->GetAnariDeviceManager();
  auto& rm = pass->GetAnariRendererManager();

  if (useDebugDevice)
  {
    vtkNew<vtkTesting> testing;
    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace/";
    traceDir += testName;
    dm.SetAnariDebugConfig(traceDir.c_str(), "code");
  }

  dm.SetupAnariDeviceFromLibrary("environment", "default", useDebugDevice);

  // General renderer parameters:
  rm.SetAnariRendererParameter("ambientRadiance", 1.f);

  // VisRTX specific renderer parameters:
  rm.SetAnariRendererParameter("lightFalloff", 0.5f);
  rm.SetAnariRendererParameter("denoise", true);
  rm.SetAnariRendererParameter("pixelSamples", 8);

  vtkAnariRendererNode::SetCompositeOnGL(renderer, 1);
}
