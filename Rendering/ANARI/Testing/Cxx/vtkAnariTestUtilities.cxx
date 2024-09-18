// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariTestUtilities.h"

#include "vtkTesting.h"

void SetAnariRendererParameterDefaults(
  vtkAnariPass* pass, vtkRenderer* renderer, bool useDebugDevice, const char* testName)
{
  if (!pass || !renderer)
    return;

  if (useDebugDevice)
  {
    vtkNew<vtkTesting> testing;
    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace/";
    traceDir += testName;
    pass->SetAnariDebugConfig(traceDir.c_str(), "code");
  }

  pass->SetupAnariDeviceFromLibrary("environment", "default", useDebugDevice);

#if 0
  vtkAnariRendererNode::SetSamplesPerPixel(6, renderer);
  vtkAnariRendererNode::SetLightFalloff(.5, renderer);
  vtkAnariRendererNode::SetUseDenoiser(1, renderer);
#endif
  vtkAnariRendererNode::SetCompositeOnGL(renderer, 1);
}
