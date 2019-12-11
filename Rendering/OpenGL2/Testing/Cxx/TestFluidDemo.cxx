/*=========================================================================

 Program:   Visualization Toolkit

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

// Define this to animation the dragon
#define ANIMATE_DRAGON

// Define this to add particle color to particles
// #define VERTEX_COLOR

//-----------------------------------------------------------------------------
// Global variables for animation pause/resume
static bool g_Animation = true;
constexpr static float g_Spacing = 2.0f * g_ParticleRadius;

#include <queue>

constexpr float colorRamp[] = { 1.0, 0.0, 0.0, 1.0, 0.5, 0.0, 1.0,
                                1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0,
                                0.0, 0.0, 1.0, 1.0, 0.0, 0.5, 1.0 };

std::array<float, 3> getColorRamp(float x)
{
  while (x > 1.0f)
  {
    x -= 1.0f;
  }
  const float segmentSize = 1.0f / 6.0f;
  float segment = floor(x / segmentSize);
  float t = (x - segmentSize * segment) / segmentSize;

  return std::array<float, 3>{ (1.0f - t) * colorRamp[int(segment) * 3] +
                                 t * colorRamp[int(segment) + 1],
                               (1.0f - t) * colorRamp[int(segment) * 3 + 1] +
                                 t * colorRamp[(int(segment) + 1) * 3 + 1],
                               (1.0f - t) * colorRamp[int(segment) * 3 + 2] +
                                 t * colorRamp[(int(segment) + 1) * 3 + 2] };
}

// Random number from [-1, 1]
float rand11()
{
  return 2.0f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) -
    1.0f;
}

#ifdef VERTEX_COLOR
static vtkNew<vtkFloatArray> g_Colors;
#endif

// Pause/resume animation by pressing spacebar
// Press 'd' to change display mode
// Press 'm' to change filter method
void keypressFunc(vtkObject* caller, unsigned long vtkNotUsed(eventId),
                  void* clientData, void* vtkNotUsed(callData))
{
  const auto iren = static_cast<vtkRenderWindowInteractor*>(caller);
  auto fluidMapper = static_cast<vtkOpenGLFluidMapper*>(clientData);
  if (iren->GetKeyCode() == ' ')
  {
    g_Animation = !g_Animation;
  }
  else if (iren->GetKeyCode() == 'd')
  {
    auto mode = static_cast<int>(fluidMapper->GetDisplayMode());
    mode = (mode + 1) % vtkOpenGLFluidMapper::NumDisplayModes;
    fluidMapper->SetDisplayMode(
      static_cast<vtkOpenGLFluidMapper::FluidDisplayMode>(mode));
    static_cast<vtkRenderWindowInteractor*>(caller)->Render();
  }
  else if (iren->GetKeyCode() == 'm')
  {
    auto filter = static_cast<int>(fluidMapper->GetSurfaceFilterMethod());
    filter = (filter + 1) % vtkOpenGLFluidMapper::NumFilterMethods;
    fluidMapper->SetSurfaceFilterMethod(
      static_cast<vtkOpenGLFluidMapper::FluidSurfaceFilterMethod>(filter));
    static_cast<vtkRenderWindowInteractor*>(caller)->Render();
  }
}

// Update particle animation data
void updateFunc(vtkObject* caller, unsigned long vtkNotUsed(eventId),
                void* clientData, void* vtkNotUsed(callData))
{
  if (!g_Animation)
  {
    return;
  }

  auto dragon = static_cast<vtkActor*>(clientData);

  // Max number of particle layers in x dimension
  constexpr static uint32_t maxLayers =
    static_cast<uint32_t>(17.0f / g_Spacing);

  // Each time step, move particles by (spacing * stepRatio) distance
  constexpr static float stepRatio = 0.5f;

  // Start position of the particles in the x dimension
  constexpr static float startX = -10.0f;

  // Min height and height variation of the fluid wave
  constexpr static int minHeight = static_cast<uint32_t>(0.8f / g_Spacing);
  constexpr static int heightVariation =
    static_cast<uint32_t>(0.65f / g_Spacing);
  constexpr static int minZ = -static_cast<int>(1.0f / g_Spacing);
  constexpr static int maxZ = static_cast<int>(6.0f / g_Spacing);

  // Speed of the fluid wave
  constexpr static float waveSpeed = 5.0f;

  // Time step size
  constexpr static float timeStep = 0.006f;

  constexpr static uint32_t maxHeight = 2*heightVariation + minHeight;
  constexpr static uint32_t maxPoints
    = maxLayers * maxHeight * (maxZ - minZ);

  static std::queue<uint32_t> layerSizeQueue;
  static uint32_t layers = 0;
  static float t = 0;
  static float lastX = startX;
  static bool allocationDone = false;

  if (!allocationDone)
  {
    g_Points->Allocate(maxPoints*3);
#ifdef VERTEX_COLOR
    g_Colors->Allocate(maxPoints*3);
#endif
    allocationDone = true;
  }

  // Remove the last fluid layer in the x dimension
  auto oldLayerSize = 0;
  if (layers > maxLayers)
  {
    oldLayerSize = layerSizeQueue.front();
    layerSizeQueue.pop();
    --layers;
  }

  // Shift particles to the right (positve x)
  auto pointsToMove = g_Points->GetNumberOfPoints() - oldLayerSize;
  float *pptr = static_cast<float *>(g_Points->GetVoidPointer(0));
  auto lpptr = pptr + pointsToMove*3;
  for (auto i = 0; i < pointsToMove; ++i)
  {
    pptr[i*3] = pptr[(i+oldLayerSize)*3] + g_Spacing * stepRatio;
    pptr[i*3 + 1] = pptr[(i+oldLayerSize)*3 + 1];
    pptr[i*3 + 2] = pptr[(i+oldLayerSize)*3 + 2];
  }

#ifdef VERTEX_COLOR
  float *cptr = static_cast<float *>(g_Colors->GetVoidPointer(0));
  auto lcptr = cptr + pointsToMove*3;
  if (oldLayerSize)
  {
    memmove(cptr, cptr+oldLayerSize*3, pointsToMove*3*sizeof(float));
  }
#endif
  lastX += g_Spacing * stepRatio;

#ifdef ANIMATE_DRAGON
  dragon->SetPosition(g_DragonPos[0],
                      g_DragonPos[1] +
                        static_cast<double>(std::cos(waveSpeed * t)) * 0.5,
                      g_DragonPos[2]);
#endif

  // Append one more layer
  uint32_t newLayerSize = 0;
  if (lastX >= startX + g_Spacing)
  {
    int height = static_cast<int>(heightVariation * std::cos(waveSpeed * t) +
                                  heightVariation) +
      minHeight;
    for (int y = 0; y < height; ++y)
    {
      for (int z = minZ; z < maxZ; ++z)
      {
        ++newLayerSize;
        *(lpptr++) = static_cast<float>(startX + 0.5f * rand11() * g_Spacing);
        *(lpptr++) = static_cast<float>((y + 0.5f * rand11()) * g_Spacing);
        *(lpptr++) = static_cast<float>((z + 0.5f * rand11()) * g_Spacing);
#ifdef VERTEX_COLOR
        const auto color = getColorRamp(t);
        *(lcptr++) = color[0];
        *(lcptr++) = color[1];
        *(lcptr++) = color[2];
#endif
      }
    }
    layerSizeQueue.push(newLayerSize);
    ++layers;
    lastX = startX;
  }

  t += timeStep;
  // points always change their position
  g_Points->Modified();
  // the number of points, and colors doesn't always change
  if (oldLayerSize > 0 ||  newLayerSize > 0)
  {
    g_Points->SetNumberOfPoints(pointsToMove + newLayerSize);
#ifdef VERTEX_COLOR
    g_Colors->SetNumberOfTuples(pointsToMove + newLayerSize);
    g_Colors->Modified();
#endif
  }

  static_cast<vtkRenderWindowInteractor*>(caller)->Render();
  //    std::cout << "Particle: " << g_Points->GetNumberOfPoints() << "\n";
  //    std::cout << "layers: " << layers << "\n";
  //    std::cout << "maxlayers: " << maxLayers << "\n";
}

void setupInteractiveDemo(
  vtkRenderWindow *renderWindow,
  vtkRenderer *renderer,
  vtkRenderWindowInteractor *iren,
#ifdef VERTEX_COLOR
  vtkPolyData *pointData,
#else
  vtkPolyData *vtkNotUsed(pointData),
#endif
  vtkActor* dragon,
  vtkOpenGLFluidMapper* fluidMapper
  )
{
  //------------------------------------------------------------
  // Create a light
  double lightPosition[3] = { -10, 10, 0 };
  double lightFocalPoint[3] = { 0, 0, 0 };

  {
  vtkNew<vtkLight> light;
  light->SetLightTypeToSceneLight();
  light->SetPosition(lightPosition[0], lightPosition[1], lightPosition[2]);
  light->SetPositional(true);
  light->SetConeAngle(30);
  light->SetFocalPoint(
    lightFocalPoint[0], lightFocalPoint[1], lightFocalPoint[2]);
  light->SetColor(1, 0.5, 0.5);
  renderer->AddLight(light);
  }

  {
  vtkNew<vtkLight> light;
  light->SetLightTypeToSceneLight();
  light->SetPosition(0, 10, 10);
  light->SetPositional(true);
  light->SetConeAngle(30);
  light->SetFocalPoint(
    lightFocalPoint[0], lightFocalPoint[1], lightFocalPoint[2]);
  light->SetColor(0.5, 1, 0.5);
  renderer->AddLight(light);
  }

#ifdef VERTEX_COLOR
  g_Colors->SetNumberOfComponents(3);
  pointData->GetPointData()->SetScalars(g_Colors);
  g_FluidMapper->ScalarVisibilityOn();
#endif

  renderWindow->SetSize(1920, 1080);
  vtkNew<vtkCallbackCommand> updateCallback;
  vtkNew<vtkCallbackCommand> keypressCallback;
  updateCallback->SetCallback(updateFunc);
  updateCallback->SetClientData(dragon);
  keypressCallback->SetCallback(keypressFunc);
  keypressCallback->SetClientData(fluidMapper);

  iren->AddObserver(vtkCommand::TimerEvent, updateCallback);
  iren->AddObserver(vtkCommand::KeyPressEvent, keypressCallback);
  iren->Initialize();
  iren->CreateRepeatingTimer(0);
}
