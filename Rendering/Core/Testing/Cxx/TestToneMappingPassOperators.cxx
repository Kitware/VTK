// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Checks each operator of vtkToneMappingPass against its formula on a surface lit dimly enough to
// be displayed without tone mapping, and that colors above 1 reach the operators. The surface is
// colored because the ACES matrices of the generic filmic operator barely change a gray.

#include "vtkActor.h"
#include "vtkCameraPass.h"
#include "vtkLight.h"
#include "vtkLightsPass.h"
#include "vtkNew.h"
#include "vtkOpaquePass.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkToneMappingPass.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>

namespace
{
using Color = std::array<double, 3>;

Color Linear(const Color& encoded)
{
  Color result;
  std::transform(encoded.begin(), encoded.end(), result.begin(),
    [](double value) { return std::pow(value, 2.2); });
  return result;
}

Color Encoded(const Color& linear)
{
  Color result;
  std::transform(linear.begin(), linear.end(), result.begin(),
    [](double value) { return std::pow(std::clamp(value, 0.0, 1.0), 1.0 / 2.2); });
  return result;
}

Color PerChannel(const Color& color, const std::function<double(double)>& function)
{
  Color result;
  std::transform(color.begin(), color.end(), result.begin(), function);
  return result;
}

// Column major, as in the shaders.
Color Multiply(const double matrix[9], const Color& color)
{
  Color result;
  for (int i = 0; i < 3; ++i)
  {
    result[i] = matrix[i] * color[0] + matrix[3 + i] * color[1] + matrix[6 + i] * color[2];
  }
  return result;
}

// The generic filmic curve of vtkToneMappingPass.
Color GenericFilmic(const Color& linear, double contrast, double shoulder, double midIn,
  double midOut, double hdrMax, bool useACES)
{
  const double a = contrast;
  const double d = shoulder;
  const double m = midIn;
  const double n = midOut;
  const double denominator = std::pow(m, a * d) * n - n * std::pow(hdrMax, a * d);
  const double numerator =
    std::pow(m, a * d) * n * std::pow(hdrMax, a) - std::pow(m, a) * std::pow(hdrMax, a * d);
  const double b = -((std::pow(m, -a * d) * (-std::pow(m, a) + (n * numerator) / denominator)) / n);
  const double c = std::max(numerator / denominator, 0.0);

  static const double acesInput[9] = { 0.5972782409, 0.0760130499, 0.0284085382, 0.3545713181,
    0.9083220973, 0.1338243154, 0.0482176639, 0.0156579968, 0.8375684636 };
  static const double acesOutput[9] = { 1.6047539945, -0.1020831870, -0.0032670420, -0.5310794927,
    1.1081322801, -0.0727552477, -0.0736720338, -0.0060518756, 1.0760219533 };
  Color toned = linear;
  if (useACES)
  {
    toned = Multiply(acesInput, toned);
  }
  for (double& value : toned)
  {
    value = std::pow(value, a) / (std::pow(value, a * d) * b + c);
  }
  if (useACES)
  {
    toned = Multiply(acesOutput, toned);
  }
  return PerChannel(toned, [](double value) { return std::clamp(value, 0.0, 1.0); });
}

// The PBR neutral operator of vtkToneMappingPass.
Color NeutralPBR(const Color& linear)
{
  const double startCompression = 0.8 - 0.04;
  const double desaturation = 0.15;
  const double x = *std::min_element(linear.begin(), linear.end());
  const double offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
  Color toned = PerChannel(linear, [&](double value) { return value - offset; });
  const double peak = *std::max_element(toned.begin(), toned.end());
  if (peak < startCompression)
  {
    return toned;
  }
  const double d = 1.0 - startCompression;
  const double newPeak = 1.0 - d * d / (peak + d - startCompression);
  const double g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
  return PerChannel(
    toned, [&](double value) { return (1.0 - g) * value * newPeak / peak + g * newPeak; });
}

double MaxDifference(const Color& a, const Color& b)
{
  double difference = 0.0;
  for (int i = 0; i < 3; ++i)
  {
    difference = std::max(difference, std::abs(a[i] - b[i]));
  }
  return difference;
}

void Print(std::ostream& os, const Color& color)
{
  os << "(" << color[0] << ", " << color[1] << ", " << color[2] << ")";
}

Color CenterColor(vtkRenderWindow* window)
{
  window->Render();
  int* size = window->GetSize();
  vtkNew<vtkUnsignedCharArray> pixel;
  window->GetRGBACharPixelData(size[0] / 2, size[1] / 2, size[0] / 2, size[1] / 2, 0, pixel);
  return { pixel->GetValue(0) / 255.0, pixel->GetValue(1) / 255.0, pixel->GetValue(2) / 255.0 };
}
}

int TestToneMappingPassOperators(int, char*[])
{
  vtkNew<vtkPlaneSource> plane;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(plane->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToPBR();
  actor->GetProperty()->SetColor(1.0, 0.7, 0.4);
  actor->GetProperty()->SetRoughness(1.0);

  vtkNew<vtkLight> light;
  light->SetLightTypeToHeadlight();
  vtkNew<vtkRenderer> renderer;
  renderer->AddLight(light);
  renderer->AddActor(actor);
  renderer->ResetCamera();
  vtkNew<vtkRenderWindow> window;
  window->SetSize(300, 300);
  window->AddRenderer(renderer);

  vtkNew<vtkLightsPass> lights;
  vtkNew<vtkOpaquePass> opaque;
  vtkNew<vtkRenderPassCollection> passes;
  passes->AddItem(lights);
  passes->AddItem(opaque);
  vtkNew<vtkSequencePass> sequence;
  sequence->SetPasses(passes);
  vtkNew<vtkCameraPass> camera;
  camera->SetDelegatePass(sequence);
  vtkNew<vtkToneMappingPass> toneMapping;
  toneMapping->SetDelegatePass(camera);

  light->SetIntensity(1.0);
  const Color none = CenterColor(window);
  if (*std::min_element(none.begin(), none.end()) <= 0.2 ||
    *std::max_element(none.begin(), none.end()) >= 0.9)
  {
    std::cerr << "The reference without tone mapping should be well within (0, 1): ";
    Print(std::cerr, none);
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }
  const Color linear = Linear(none);

  struct Case
  {
    const char* Name;
    std::function<void()> Configure;
    Color Expected;
  };
  const Case cases[] = {
    { "Clamp", [&] { toneMapping->SetToneMappingType(vtkToneMappingPass::Clamp); }, none },
    { "Reinhard", [&] { toneMapping->SetToneMappingType(vtkToneMappingPass::Reinhard); },
      Encoded(PerChannel(linear, [](double value) { return value / (1.0 + value); })) },
    { "Exponential",
      [&]
      {
        toneMapping->SetToneMappingType(vtkToneMappingPass::Exponential);
        toneMapping->SetExposure(2.0);
      },
      Encoded(PerChannel(linear, [](double value) { return 1.0 - std::exp(-2.0 * value); })) },
    { "GenericFilmic",
      [&]
      {
        toneMapping->SetToneMappingType(vtkToneMappingPass::GenericFilmic);
        toneMapping->SetExposure(1.0);
        toneMapping->SetGenericFilmicDefaultPresets();
      },
      Encoded(GenericFilmic(linear, 1.6773, 0.9714, 0.18, 0.18, 11.0785, true)) },
    { "GenericFilmic Uncharted 2",
      [&]
      {
        toneMapping->SetToneMappingType(vtkToneMappingPass::GenericFilmic);
        toneMapping->SetGenericFilmicUncharted2Presets();
      },
      Encoded(GenericFilmic(linear, 1.1759, 0.9746, 0.18, 0.18, 6.3704, false)) },
    { "NeutralPBR", [&] { toneMapping->SetToneMappingType(vtkToneMappingPass::NeutralPBR); },
      Encoded(NeutralPBR(linear)) },
  };

  bool success = true;
  renderer->SetPass(toneMapping);
  for (const Case& test : cases)
  {
    test.Configure();
    const Color value = CenterColor(window);
    if (MaxDifference(value, test.Expected) > 2.0 / 255.0)
    {
      std::cerr << test.Name << ": ";
      Print(std::cerr, value);
      std::cerr << ", expected ";
      Print(std::cerr, test.Expected);
      std::cerr << std::endl;
      success = false;
    }
  }

  // A color clipped to 1 before tone mapping would give Reinhard at most 1 / 2.
  light->SetIntensity(20.0);
  toneMapping->SetToneMappingType(vtkToneMappingPass::Reinhard);
  const double bright = CenterColor(window)[0];
  if (bright <= std::pow(0.5, 1.0 / 2.2) + 0.02)
  {
    std::cerr << "Colors above 1 do not reach the operator: Reinhard gives " << bright
              << " in red under a bright light." << std::endl;
    success = false;
  }

  toneMapping->ReleaseGraphicsResources(window);
  renderer->SetPass(nullptr);
  light->SetIntensity(1.0);
  const Color noneAgain = CenterColor(window);
  if (MaxDifference(noneAgain, none) > 1.0 / 255.0)
  {
    std::cerr << "Without the pass again: ";
    Print(std::cerr, noneAgain);
    std::cerr << ", expected ";
    Print(std::cerr, none);
    std::cerr << std::endl;
    success = false;
  }

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
