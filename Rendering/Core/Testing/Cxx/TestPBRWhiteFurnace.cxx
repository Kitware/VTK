// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test is a furnace test for the PBR image based lighting path.
// A white sphere sits in an environment of uniform radiance and no other light, so a material
// that conserves energy can reflect at most that radiance back at the camera. Every material
// is checked against that bound, which catches a lobe that is added without taking its energy
// from the lobe underneath it. The test runs for every rendering backend, so the backends are
// held to the same bound.
//
// The furnace radiance is 0.5 rather than 1.0 on purpose: the color buffer is 8 bit, so a
// material that gains energy would saturate at 1.0 and the gain would be invisible.
//
// The bound is one directional: the model is allowed to lose energy, because VTK's split sum
// approximation and its multiple scattering compensation are approximations, and a rough
// dielectric legitimately returns less than it receives.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTexture.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace
{

// radiance of the furnace, see the note above on why it is not 1.0
constexpr double FurnaceRadiance = 0.5;

// how much more than the furnace radiance a material may reflect. It covers the 8 bit
// quantization of the color buffer and the error of the BRDF lookup table, not a real gain.
constexpr double GainTolerance = 0.04;

// how much less than the furnace radiance a material may reflect before it looks like the
// environment stopped reaching it at all. It is deliberately loose: a rough coat loses a lot
// of energy because the coat lobe, unlike the base layer, has no multiple scattering
// compensation. The bound that this test is really about is the one above it.
constexpr double LossTolerance = 0.65;

// the window is square and the camera is parallel, so the sphere projects to a disk of a
// known radius and the furnace pixels can be found without classifying them by color
constexpr int WindowSize = 200;
constexpr double SphereRadius = 0.5;
constexpr double ParallelScale = 1.0;

struct Material
{
  std::string Name;
  double Metallic;
  double Roughness;
  double CoatStrength;
  double CoatRoughness;
};

vtkSmartPointer<vtkTexture> MakeFurnaceTexture()
{
  // an equirectangular environment of uniform radiance
  vtkNew<vtkImageData> image;
  image->SetDimensions(128, 64, 1);
  image->AllocateScalars(VTK_FLOAT, 3);

  float* scalars = static_cast<float*>(image->GetScalarPointer());
  const vtkIdType count = 128 * 64 * 3;
  for (vtkIdType i = 0; i < count; ++i)
  {
    scalars[i] = static_cast<float>(FurnaceRadiance);
  }

  vtkNew<vtkTexture> texture;
  texture->SetColorModeToDirectScalars();
  texture->MipmapOn();
  texture->InterpolateOn();
  texture->SetInputData(image);
  return texture;
}

// Report the brightest and the dimmest radiance the sphere reflects, measured over the
// interior of the disk it projects to.
bool MeasureFurnace(vtkRenderWindow* renWin, const std::string& name)
{
  vtkNew<vtkUnsignedCharArray> pixels;
  if (!renWin->GetPixelData(0, 0, WindowSize - 1, WindowSize - 1, 1, pixels))
  {
    std::cerr << name << ": could not read the color buffer back\n";
    return false;
  }

  const double pixelsPerUnit = 0.5 * WindowSize / ParallelScale;
  // stay off the silhouette, where the sphere only partly covers a pixel
  const double diskRadius = 0.98 * SphereRadius * pixelsPerUnit;
  const double center = 0.5 * (WindowSize - 1);

  double brightest = 0.0;
  double dimmest = 1.0;
  vtkIdType measured = 0;

  for (int y = 0; y < WindowSize; ++y)
  {
    for (int x = 0; x < WindowSize; ++x)
    {
      const double dx = x - center;
      const double dy = y - center;
      if (dx * dx + dy * dy > diskRadius * diskRadius)
      {
        continue;
      }
      ++measured;
      for (int c = 0; c < 3; ++c)
      {
        const double encoded = pixels->GetTypedComponent((y * WindowSize) + x, c) / 255.0;
        // the PBR shaders encode the color with a 2.2 gamma
        const double radiance = std::pow(encoded, 2.2);
        brightest = std::max(brightest, radiance);
        dimmest = std::min(dimmest, radiance);
      }
    }
  }

  if (measured == 0)
  {
    std::cerr << name << ": the sphere did not cover any of the measured pixels\n";
    return false;
  }

  const double gain = brightest / FurnaceRadiance;
  const double loss = dimmest / FurnaceRadiance;
  std::cout << name << ": brightest " << brightest << " (" << gain << " of the furnace), dimmest "
            << dimmest << " (" << loss << " of the furnace), over " << measured << " pixels\n";

  bool ok = true;
  if (gain > 1.0 + GainTolerance)
  {
    std::cerr << name << ": reflects " << gain << " of the furnace radiance, which is more energy"
              << " than it receives (bound " << 1.0 + GainTolerance << ")\n";
    ok = false;
  }
  if (loss < 1.0 - LossTolerance)
  {
    std::cerr << name << ": reflects only " << loss << " of the furnace radiance (bound "
              << 1.0 - LossTolerance << "), the environment barely reaches it\n";
    ok = false;
  }
  return ok;
}

} // anonymous namespace

//------------------------------------------------------------------------------
int TestPBRWhiteFurnace(int, char*[])
{
  vtkNew<vtkRenderer> renderer;
  // no light of any kind: the furnace is the only source, which leaves the image based
  // lighting path on its own
  renderer->AutomaticLightCreationOff();
  renderer->SetEnvironmentTexture(MakeFurnaceTexture());
  renderer->UseImageBasedLightingOn();
  renderer->UseSphericalHarmonicsOff();
  renderer->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(WindowSize, WindowSize);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(SphereRadius);
  sphere->SetThetaResolution(100);
  sphere->SetPhiResolution(100);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  vtkProperty* property = actor->GetProperty();
  property->SetInterpolationToPBR();
  property->SetColor(1.0, 1.0, 1.0);
  renderer->AddActor(actor);

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->ParallelProjectionOn();
  camera->SetParallelScale(ParallelScale);
  camera->SetPosition(0.0, 0.0, 3.0);
  camera->SetFocalPoint(0.0, 0.0, 0.0);
  camera->SetViewUp(0.0, 1.0, 0.0);

  const Material materials[] = {
    { "smooth metal", 1.0, 0.05, 0.0, 0.0 },
    { "rough metal", 1.0, 1.0, 0.0, 0.0 },
    { "smooth dielectric", 0.0, 0.1, 0.0, 0.0 },
    { "rough dielectric", 0.0, 1.0, 0.0, 0.0 },
    { "coated smooth metal", 1.0, 0.1, 1.0, 0.05 },
    { "coated rough metal", 1.0, 0.8, 1.0, 0.3 },
    { "coated dielectric", 0.0, 0.5, 1.0, 0.05 },
  };

  bool ok = true;
  for (const Material& material : materials)
  {
    property->SetMetallic(material.Metallic);
    property->SetRoughness(material.Roughness);
    property->SetCoatStrength(material.CoatStrength);
    property->SetCoatRoughness(material.CoatRoughness);
    renWin->Render();
    ok = MeasureFurnace(renWin, material.Name) && ok;
  }

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
