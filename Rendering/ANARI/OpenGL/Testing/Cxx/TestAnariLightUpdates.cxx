// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAnariDevice.h"
#include "vtkAnariLightNode.h"
#include "vtkAnariOpenGLTestUtilities.h"
#include "vtkAnariPass.h"
#include "vtkAnariRenderer.h"
#include "vtkAnariSceneGraph.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkLogger.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkViewNodeFactory.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace
{
class vtkTestAnariSceneGraph : public vtkAnariSceneGraph
{
public:
  static vtkTestAnariSceneGraph* New();
  vtkTypeMacro(vtkTestAnariSceneGraph, vtkAnariSceneGraph);

  vtkMTimeType GetStructureTime() { return this->AnariSceneStructureModifiedMTime; }

  void RenderLightUpdates()
  {
    this->Traverse(vtkViewNode::build);
    for (auto* child : this->GetChildren())
    {
      if (child->IsA("vtkAnariLightNode") || child->IsA("vtkAnariCameraNode"))
      {
        child->Traverse(vtkViewNode::synchronize);
      }
    }
    this->Traverse(vtkViewNode::render);
  }
};
vtkStandardNewMacro(vtkTestAnariSceneGraph);

vtkViewNode* MakeSceneGraph()
{
  return vtkTestAnariSceneGraph::New();
}

double GetCenterBrightness(vtkAnariSceneGraph* graph, vtkRenderWindow* window)
{
  const auto* pixels = graph->GetBuffer();
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

  vtkNew<vtkRenderWindow> window;
  vtkNew<vtkRenderer> renderer;
  window->AddRenderer(renderer);
  window->SetSize(100, 100);
  window->SetMultiSamples(0);
  window->SetOffScreenRendering(true);
  vtkNew<vtkAnariPass> pass;
  pass->GetViewNodeFactory()->RegisterOverride("vtkRenderer", MakeSceneGraph);
  vtkAnariOpenGLTestUtilities::SetParameterDefaults(
    pass, renderer, useDebugDevice, "TestAnariLightUpdates");
  pass->GetAnariRenderer()->SetParameterf("ambientRadiance", 0.0f);
  pass->GetAnariRenderer()->SetParameterb("denoise", false);
  renderer->SetPass(pass);

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

  auto* graph = vtkTestAnariSceneGraph::SafeDownCast(pass->GetSceneGraph());
  auto* headlight = vtkLight::SafeDownCast(renderer->GetLights()->GetItemAsObject(0));
  if (!headlight || !headlight->LightTypeIsHeadlight())
  {
    vtkLogF(ERROR, "The ANARI scene did not create its default headlight.");
    return EXIT_FAILURE;
  }

  bool success = true;
  auto structureTime = graph->GetStructureTime();
  auto checkStructure = [&](bool changed, const char* operation)
  {
    graph->RenderLightUpdates();
    const auto nextTime = graph->GetStructureTime();
    if ((nextTime != structureTime) != changed)
    {
      vtkLogF(ERROR, "Unexpected scene structure update after %s.", operation);
      success = false;
    }
    structureTime = nextTime;
  };

  const auto& extensions = pass->GetAnariDevice()->GetExtensions();
  if (!extensions.ANARI_KHR_LIGHT_DIRECTIONAL)
  {
    vtkLogF(WARNING, "Directional lights are required for this test.");
    return 125;
  }

  if (GetCenterBrightness(graph, window) < 100.0)
  {
    vtkLogF(ERROR, "The default headlight did not illuminate the sphere.");
    success = false;
  }
  renderer->GetActiveCamera()->Azimuth(180.0);
  checkStructure(false, "moving the headlight with the camera");
  if (GetCenterBrightness(graph, window) < 100.0)
  {
    vtkLogF(ERROR, "The headlight did not follow the camera.");
    success = false;
  }

  headlight->SetLightTypeToCameraLight();
  headlight->SetPosition(0.0, 0.0, 1.0);
  headlight->SetFocalPoint(0.0, 0.0, 0.0);
  checkStructure(false, "changing to a camera light");
  renderer->GetActiveCamera()->Azimuth(180.0);
  checkStructure(false, "moving the camera light with the camera");
  if (GetCenterBrightness(graph, window) < 100.0)
  {
    vtkLogF(ERROR, "The camera light did not follow the camera.");
    success = false;
  }

  auto* lightNode = graph->GetViewNodeFor(headlight);
  const auto lightTime = lightNode->RenderTime;
  checkStructure(false, "rendering an unchanged light");
  if (lightNode->RenderTime != lightTime)
  {
    vtkLogF(ERROR, "An unchanged light was synchronized again.");
    success = false;
  }

  headlight->SwitchOff();
  checkStructure(false, "switching off the light");
  if (GetCenterBrightness(graph, window) > 10.0)
  {
    vtkLogF(ERROR, "Switching off the retained light did not darken the sphere.");
    success = false;
  }
  headlight->SwitchOn();
  checkStructure(false, "switching on the light");
  vtkAnariLightNode::SetLightScale(0.0, headlight);
  checkStructure(false, "changing the light scale information key");
  if (GetCenterBrightness(graph, window) > 10.0)
  {
    vtkLogF(ERROR, "The retained light ignored its light scale information key.");
    success = false;
  }
  vtkAnariLightNode::SetLightScale(1.0, headlight);
  headlight->SetIntensity(0.5);
  headlight->SetColor(1.0, 0.5, 0.25);
  vtkAnariLightNode::SetRadius(0.05, headlight);
  checkStructure(false, "changing light color, intensity, and radius");

  vtkNew<vtkTransform> transform;
  renderer->SetUserLightTransform(transform);
  checkStructure(false, "setting a user light transform");
  const auto transformTime = lightNode->RenderTime;
  transform->RotateY(180.0);
  checkStructure(false, "modifying the user light transform");
  if (lightNode->RenderTime == transformTime)
  {
    vtkLogF(ERROR, "The retained light ignored its modified user transform.");
    success = false;
  }
  if (GetCenterBrightness(graph, window) > 10.0)
  {
    vtkLogF(ERROR, "The modified user transform did not turn the light away from the sphere.");
    success = false;
  }
  renderer->SetUserLightTransform(nullptr);
  checkStructure(false, "removing the user light transform");

  headlight->SetLightTypeToSceneLight();
  headlight->SetPosition(0.0, 0.0, 1.0);
  vtkNew<vtkMatrix4x4> lightTransform;
  headlight->SetTransformMatrix(lightTransform);
  checkStructure(false, "setting a scene light transform");
  lightTransform->SetElement(2, 2, -1.0);
  checkStructure(false, "modifying the scene light transform");
  if (GetCenterBrightness(graph, window) > 10.0)
  {
    vtkLogF(ERROR, "The retained scene light ignored its modified transform matrix.");
    success = false;
  }
  const auto sceneLightTime = lightNode->RenderTime;
  renderer->GetActiveCamera()->Roll(15.0);
  checkStructure(false, "moving the camera with a scene light");
  if (lightNode->RenderTime != sceneLightTime)
  {
    vtkLogF(ERROR, "Camera motion synchronized a scene light.");
    success = false;
  }
  headlight->SetLightTypeToCameraLight();
  checkStructure(false, "restoring the camera light");

  if (extensions.ANARI_KHR_LIGHT_POINT)
  {
    headlight->SetPositional(true);
    headlight->SetConeAngle(90.0);
    checkStructure(true, "changing the light subtype to point");
    headlight->SetPosition(0.5, 0.0, 1.0);
    checkStructure(false, "moving the point light");
  }
  if (extensions.ANARI_KHR_LIGHT_SPOT)
  {
    headlight->SetPositional(true);
    headlight->SetConeAngle(30.0);
    checkStructure(true, "changing the light subtype to spot");
    headlight->SetConeAngle(40.0);
    vtkAnariLightNode::SetFalloffAngle(0.2, headlight);
    checkStructure(false, "changing the spot opening and falloff angles");
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
    checkStructure(true, "changing the light subtype to HDRI");
    const double initialRadiance = GetCenterBrightness(graph, window);
    renderer->GetActiveCamera()->Azimuth(15.0);
    checkStructure(false, "moving the HDRI light with the camera");
    std::fill(values, values + 8 * 4 * 3, 0.2f);
    image->Modified();
    checkStructure(false, "changing the HDRI radiance");
    if (GetCenterBrightness(graph, window) < initialRadiance * 1.2 + 1.0)
    {
      vtkLogF(ERROR, "The retained HDRI light ignored its modified radiance.");
      success = false;
    }
    renderer->UseImageBasedLightingOff();
    checkStructure(true, "disabling image based lighting");
  }

  headlight->SetPositional(false);
  checkStructure(extensions.ANARI_KHR_LIGHT_POINT || extensions.ANARI_KHR_LIGHT_SPOT,
    "restoring the directional light subtype");
  vtkNew<vtkLight> additionalLight;
  additionalLight->SetLightTypeToSceneLight();
  renderer->AddLight(additionalLight);
  checkStructure(true, "adding a light");
  renderer->RemoveLight(additionalLight);
  checkStructure(true, "removing a light");

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
