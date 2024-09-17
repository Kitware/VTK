// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariTestUtilities.h"

#include "vtkTesting.h"

void SetAnariRendererParameterDefaults(
  vtkRenderer* renderer, bool useDebugDevice, const char* testName)
{
  if (!renderer)
    return;

#if 0
  vtkAnariRendererNode::SetLibraryName(renderer, "environment");
  vtkAnariRendererNode::SetSamplesPerPixel(6, renderer);
  vtkAnariRendererNode::SetLightFalloff(.5, renderer);
  vtkAnariRendererNode::SetUseDenoiser(1, renderer);
#endif
  vtkAnariRendererNode::SetCompositeOnGL(renderer, 1);

  if (useDebugDevice)
  {
#if 0
    vtkAnariRendererNode::SetUseDebugDevice(renderer, 1);
    vtkNew<vtkTesting> testing;

    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace/";
    traceDir += testName;
    vtkAnariRendererNode::SetDebugDeviceDirectory(renderer, traceDir.c_str());
#endif
  }
}
