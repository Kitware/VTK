// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariOpenGLTestUtilities.h"

#include "vtkTesting.h"

#include "vtkAnariDevice.h"
#include "vtkAnariPass.h"
#include "vtkAnariRenderWindow.h"
#include "vtkAnariRenderer.h"
#include "vtkAnariSceneGraph.h"

namespace vtkAnariOpenGLTestUtilities
{

void SetParameterDefaults(
  vtkAnariPass* anariPass, vtkRenderer* renderer, bool useDebugDevice, const char* testName)
{
  auto* anariDevice = anariPass->GetAnariDevice();
  auto* anariRenderer = anariPass->GetAnariRenderer();

  if (useDebugDevice)
  {
    vtkNew<vtkTesting> testing;
    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace/";
    traceDir += testName;
    anariDevice->SetAnariDebugConfig(traceDir.c_str(), "code");
  }

  anariDevice->SetupAnariDeviceFromLibrary("environment", "default", useDebugDevice);

  // General renderer parameters:
  anariRenderer->SetParameterf("ambientRadiance", 1.f);

  // VisRTX specific renderer parameters:
  anariRenderer->SetParameterf("lightFalloff", 0.5f);
  anariRenderer->SetParameterb("denoise", true);
  anariRenderer->SetParameteri("pixelSamples", 8);

  if (renderer)
  {
    vtkAnariSceneGraph::SetCompositeOnGL(renderer, 1);
  }
}

const anari::Extensions& GetDeviceExtensions(vtkAnariPass* anariPass)
{
  return anariPass->GetAnariDevice()->GetAnariDeviceExtensions();
}

}
