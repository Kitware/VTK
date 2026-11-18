// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAnariDevice.h"
#include "vtkAnariLightNode.h"
#include "vtkAnariRenderWindow.h"
#include "vtkAnariRenderer.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkLogger.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace
{
double GetCenterBrightness(vtkAnariRenderWindow* window)
{
  const auto* pixels = window->GetSceneGraph()->GetBuffer();
  const auto* size = window->GetSize();
  const int xMin = std::max(size[0] / 2 - 2, 0);
  const int xMax = std::min(size[0] / 2 + 2, size[0] - 1);
  const int yMin = std::max(size[1] / 2 - 2, 0);
  const int yMax = std::min(size[1] / 2 + 2, size[1] - 1);
  double brightness = 0.0;
  for (int y = yMin; y <= yMax; ++y)
  {
    for (int x = xMin; x <= xMax; ++x)
    {
      const auto* pixel = pixels + (y * size[0] + x) * 4;
      brightness += pixel[0] + pixel[1] + pixel[2];
    }
  }
  const int sampledPixels = (xMax - xMin + 1) * (yMax - yMin + 1);
  return brightness / (3.0 * sampledPixels);
}
}

int TestAnariLightUpdates(int argc, char* argv[])
{
  bool useDebugDevice = false;
  for (int i = 0; i < argc; ++i)
  {
    useDebugDevice |= std::strcmp(argv[i], "--trace") == 0;
  }

  vtkNew<vtkAnariRenderWindow> window;
  vtkNew<vtkRenderer> renderer;
  window->AddRenderer(renderer);
  window->SetSize(100, 100);
  window->SetMultiSamples(0);
  window->SetOffScreenRendering(true);
  vtkAnariTestUtilities::SetParameterDefaults(window, useDebugDevice, "TestAnariLightUpdates");
  window->GetAnariRenderer()->SetParameterf("ambientRadiance", 0.0f);
  window->GetAnariRenderer()->SetParameterb("denoise", false);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(40);
  sphere->SetPhiResolution(40);
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetAmbient(0.0);
  actor->GetProperty()->SetDiffuse(1.0);
  renderer->AddActor(actor);
  renderer->ResetCamera();
  window->Render();

  auto* graph = window->GetSceneGraph();
  if (!graph)
  {
    vtkLogF(ERROR, "The ANARI render window did not create its scene graph.");
    return EXIT_FAILURE;
  }
  auto* headlight = vtkLight::SafeDownCast(renderer->GetLights()->GetItemAsObject(0));
  if (!headlight || !headlight->LightTypeIsHeadlight())
  {
    vtkLogF(ERROR, "The ANARI scene did not create its default headlight.");
    return EXIT_FAILURE;
  }

  bool success = true;

  const auto& extensions = window->GetAnariDevice()->GetExtensions();
  if (!extensions.ANARI_KHR_LIGHT_DIRECTIONAL)
  {
    vtkLogF(WARNING, "Directional lights are required for this test.");
    return VTK_SKIP_RETURN_CODE;
  }

  if (GetCenterBrightness(window) < 100.0)
  {
    vtkLogF(ERROR, "The default headlight did not illuminate the sphere.");
    success = false;
  }
  renderer->GetActiveCamera()->Azimuth(180.0);
  window->Render();
  if (GetCenterBrightness(window) < 100.0)
  {
    vtkLogF(ERROR, "The headlight did not follow the camera.");
    success = false;
  }

  headlight->SetLightTypeToCameraLight();
  headlight->SetPosition(0.0, 0.0, 1.0);
  headlight->SetFocalPoint(0.0, 0.0, 0.0);
  window->Render();
  renderer->GetActiveCamera()->Azimuth(180.0);
  window->Render();
  if (GetCenterBrightness(window) < 100.0)
  {
    vtkLogF(ERROR, "The camera light did not follow the camera.");
    success = false;
  }

  auto* lightNode = graph->GetViewNodeFor(headlight);
  const auto lightTime = lightNode->RenderTime;
  window->Render();
  if (lightNode->RenderTime != lightTime)
  {
    vtkLogF(ERROR, "An unchanged light was synchronized again.");
    success = false;
  }

  headlight->SwitchOff();
  window->Render();
  if (GetCenterBrightness(window) > 10.0)
  {
    vtkLogF(ERROR, "Switching off the retained light did not darken the sphere.");
    success = false;
  }
  headlight->SwitchOn();
  window->Render();
  vtkAnariLightNode::SetLightScale(0.0, headlight);
  window->Render();
  if (GetCenterBrightness(window) > 10.0)
  {
    vtkLogF(ERROR, "The retained light ignored its light scale information key.");
    success = false;
  }
  vtkAnariLightNode::SetLightScale(1.0, headlight);
  headlight->SetIntensity(0.5);
  headlight->SetColor(1.0, 0.5, 0.25);
  vtkAnariLightNode::SetRadius(0.05, headlight);
  window->Render();

  vtkNew<vtkTransform> transform;
  renderer->SetUserLightTransform(transform);
  window->Render();
  const auto transformTime = lightNode->RenderTime;
  transform->RotateY(180.0);
  window->Render();
  if (lightNode->RenderTime == transformTime)
  {
    vtkLogF(ERROR, "The retained light ignored its modified user transform.");
    success = false;
  }
  if (GetCenterBrightness(window) > 10.0)
  {
    vtkLogF(ERROR, "The modified user transform did not turn the light away from the sphere.");
    success = false;
  }
  renderer->SetUserLightTransform(nullptr);
  window->Render();

  renderer->LightFollowCameraOff();
  transform->Identity();
  transform->RotateY(45.0);
  renderer->SetUserLightTransform(transform);
  vtkSmartPointer<vtkCamera> newerCamera = renderer->GetActiveCamera();
  vtkNew<vtkCamera> olderCamera;
  olderCamera->DeepCopy(newerCamera);
  olderCamera->Roll(90.0);
  newerCamera->Modified();
  window->Render();

  renderer->SetActiveCamera(olderCamera);
  window->Render();
  if (lightNode->RenderTime != olderCamera->GetMTime())
  {
    vtkLogF(ERROR, "The retained light ignored the replacement camera with an older MTime.");
    success = false;
  }

  const auto replacementCameraTime = olderCamera->GetMTime();
  olderCamera->Roll(30.0);
  window->Render();
  if (lightNode->RenderTime <= replacementCameraTime)
  {
    vtkLogF(ERROR, "The retained light ignored movement of the same replacement camera.");
    success = false;
  }

  renderer->SetActiveCamera(newerCamera);
  renderer->LightFollowCameraOn();
  renderer->SetUserLightTransform(nullptr);
  window->Render();

  headlight->SetLightTypeToSceneLight();
  headlight->SetPosition(0.0, 0.0, 1.0);
  vtkNew<vtkMatrix4x4> lightTransform;
  headlight->SetTransformMatrix(lightTransform);
  window->Render();
  lightTransform->SetElement(2, 2, -1.0);
  window->Render();
  if (GetCenterBrightness(window) > 10.0)
  {
    vtkLogF(ERROR, "The retained scene light ignored its modified transform matrix.");
    success = false;
  }
  const auto sceneLightTime = lightNode->RenderTime;
  renderer->GetActiveCamera()->Roll(15.0);
  window->Render();
  if (lightNode->RenderTime != sceneLightTime)
  {
    vtkLogF(ERROR, "Camera motion synchronized a scene light.");
    success = false;
  }
  headlight->SetLightTypeToCameraLight();
  window->Render();

  if (extensions.ANARI_KHR_LIGHT_POINT)
  {
    headlight->SetPositional(true);
    headlight->SetConeAngle(90.0);
    window->Render();
    headlight->SetPosition(0.5, 0.0, 1.0);
    window->Render();
  }
  if (extensions.ANARI_KHR_LIGHT_SPOT)
  {
    headlight->SetPositional(true);
    headlight->SetConeAngle(30.0);
    window->Render();
    headlight->SetConeAngle(40.0);
    vtkAnariLightNode::SetFalloffAngle(0.2, headlight);
    window->Render();
  }

  if (extensions.ANARI_KHR_LIGHT_HDRI)
  {
    vtkNew<vtkImageData> image;
    image->SetDimensions(8, 4, 1);
    image->AllocateScalars(VTK_FLOAT, 3);
    auto* values = static_cast<float*>(image->GetScalarPointer());
    std::fill(values, values + 8 * 4 * 3, 0.1f);
    vtkNew<vtkTexture> texture;
    texture->SetInputData(image);
    renderer->SetEnvironmentTexture(texture);
    renderer->UseImageBasedLightingOn();
    window->Render();
    const double initialRadiance = GetCenterBrightness(window);
    renderer->GetActiveCamera()->Azimuth(15.0);
    window->Render();
    std::fill(values, values + 8 * 4 * 3, 0.2f);
    image->Modified();
    window->Render();
    if (GetCenterBrightness(window) < initialRadiance * 1.2 + 1.0)
    {
      vtkLogF(ERROR, "The retained HDRI light ignored its modified radiance.");
      success = false;
    }
    renderer->UseImageBasedLightingOff();
    window->Render();
  }

  headlight->SetPositional(false);
  window->Render();
  vtkNew<vtkLight> additionalLight;
  additionalLight->SetLightTypeToSceneLight();
  renderer->AddLight(additionalLight);
  window->Render();
  renderer->RemoveLight(additionalLight);
  window->Render();

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
