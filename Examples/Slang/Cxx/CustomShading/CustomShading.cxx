// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Customizes vtkWebGPUPolyDataMapper's shaders with the Slang types in custom_shading.slang.
// Press a key to switch the torus between:
//   g  the WGSL template              d  the Slang program with its default types
//   h  Stripes (IShaderHooks)         l  Toon (ILightingModel)
//   b  both
//
// Usage: CustomShading [--test] [directory of custom_shading.slang]
// With --test, the example renders every mode offscreen and checks the images instead.

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkParametricFunctionSource.h"
#include "vtkParametricTorus.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSlangShaderManager.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGPUShaderProperty.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr int SkipReturnCode = 125;
constexpr int Size = 400;

struct Mode
{
  char Key;
  const char* Name;
  bool UseSlang;
  std::vector<std::pair<const char*, const char*>> Specializations;
};

const std::vector<Mode> Modes = {
  { 'g', "the WGSL template", false, {} },
  { 'd', "the Slang program with its default types", true, {} },
  { 'h', "Stripes (IShaderHooks)", true, { { "IShaderHooks", "Stripes" } } },
  { 'l', "Toon (ILightingModel)", true, { { "ILightingModel", "Toon" } } },
  { 'b', "Stripes and Toon", true,
    { { "IShaderHooks", "Stripes" }, { "ILightingModel", "Toon" } } },
};

struct Scene
{
  vtkWebGPUShaderProperty* Shader = nullptr;
  std::string Module;
  vtkRenderWindow* Window = nullptr;
};

// A module on its own turns the Slang shaders on, so the WGSL mode clears it.
void Apply(const Scene& scene, const Mode& mode)
{
  scene.Shader->RemoveAllSlangSpecializations();
  scene.Shader->SetSlangModule(mode.UseSlang ? scene.Module.c_str() : nullptr);
  scene.Shader->SetUseSlang(mode.UseSlang);
  for (const auto& specialization : mode.Specializations)
  {
    scene.Shader->SetSlangSpecialization(specialization.first, specialization.second);
  }
  std::cout << "Rendering " << mode.Name << "\n";
}

void OnKeyPress(vtkObject* caller, unsigned long, void* clientData, void*)
{
  auto* interactor = static_cast<vtkRenderWindowInteractor*>(caller);
  const auto* scene = static_cast<Scene*>(clientData);
  for (const Mode& mode : Modes)
  {
    if (interactor->GetKeyCode() == mode.Key)
    {
      Apply(*scene, mode);
      scene->Window->Render();
    }
  }
}

double DifferingFraction(vtkUnsignedCharArray* a, vtkUnsignedCharArray* b)
{
  vtkIdType differing = 0;
  for (vtkIdType i = 0; i < a->GetNumberOfValues(); i += 3)
  {
    for (int c = 0; c < 3; ++c)
    {
      if (std::abs(a->GetValue(i + c) - b->GetValue(i + c)) > 1)
      {
        ++differing;
        break;
      }
    }
  }
  return static_cast<double>(differing) / (a->GetNumberOfValues() / 3);
}

// The default types must render like the WGSL template, and each customization must change it.
int Test(const Scene& scene, vtkObject* mapper)
{
  vtkNew<vtkCallbackCommand> onError;
  bool errors = false;
  onError->SetClientData(&errors);
  onError->SetCallback([](vtkObject*, unsigned long, void* clientData, void*)
    { *static_cast<bool*>(clientData) = true; });
  mapper->AddObserver(vtkCommand::ErrorEvent, onError);

  std::vector<vtkSmartPointer<vtkUnsignedCharArray>> images;
  for (const Mode& mode : Modes)
  {
    Apply(scene, mode);
    scene.Window->Render();
    images.emplace_back(vtkSmartPointer<vtkUnsignedCharArray>::New());
    scene.Window->GetPixelData(0, 0, Size - 1, Size - 1, 1, images.back());
  }
  bool success = !errors;
  if (errors)
  {
    std::cerr << "The mapper reported an error.\n";
  }
  if (DifferingFraction(images[0], images[1]) > 0.001)
  {
    std::cerr << "The default Slang types do not render like the WGSL template.\n";
    success = false;
  }
  for (std::size_t i = 2; i < images.size(); ++i)
  {
    if (DifferingFraction(images[1], images[i]) < 0.01)
    {
      std::cerr << Modes[i].Name << " did not change the image.\n";
      success = false;
    }
  }
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
}

int main(int argc, char* argv[])
{
  bool test = false;
  std::string directory = CUSTOM_SHADING_MODULE_DIR;
  for (int i = 1; i < argc; ++i)
  {
    if (std::string(argv[i]) == "--test")
    {
      test = true;
    }
    else
    {
      directory = argv[i];
    }
  }

  vtkNew<vtkParametricTorus> torus;
  torus->SetRingRadius(1.0);
  torus->SetCrossSectionRadius(0.45);
  vtkNew<vtkParametricFunctionSource> source;
  source->SetParametricFunction(torus);
  source->SetUResolution(160);
  source->SetVResolution(80);
  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection(source->GetOutputPort());
  normals->SplittingOff();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(normals->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->RotateX(-55);
  actor->GetProperty()->SetColor(0.9, 0.55, 0.25);
  actor->GetProperty()->SetSpecular(0.35);
  actor->GetProperty()->SetSpecularPower(40);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.08, 0.1, 0.14);
  renderer->ResetCamera();
  vtkNew<vtkRenderWindow> window;
  window->SetSize(Size, Size);
  window->SetWindowName("CustomShading");
  window->AddRenderer(renderer);

  Scene scene;
  scene.Shader = vtkWebGPUShaderProperty::SafeDownCast(actor->GetShaderProperty());
  scene.Window = window;
  if (!scene.Shader || !vtkSlangShaderManager::IsSlangAvailable())
  {
    std::cerr << "This example needs VTK's WebGPU backend built with Slang (VTK_USE_SLANG).\n";
    return test ? SkipReturnCode : EXIT_FAILURE;
  }

  // custom_shading.slang imports custom_palette.slang from its own directory.
  scene.Shader->AddSlangSearchPath(directory.c_str());
  if (!scene.Shader->LoadSlangModule((directory + "/custom_shading.slang").c_str()))
  {
    return EXIT_FAILURE;
  }
  scene.Module = scene.Shader->GetSlangModule();

  if (test)
  {
    window->SetShowWindow(false);
    window->SetUseOffScreenBuffers(true);
    return Test(scene, mapper);
  }

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  interactor->SetInteractorStyle(style);
  vtkNew<vtkCallbackCommand> onKeyPress;
  onKeyPress->SetCallback(OnKeyPress);
  onKeyPress->SetClientData(&scene);
  interactor->AddObserver(vtkCommand::KeyPressEvent, onKeyPress);

  std::cout << "Keys: g WGSL template, d Slang default types, h Stripes, l Toon, b both\n";
  Apply(scene, Modes[0]);
  window->Render();
  interactor->Start();
  return EXIT_SUCCESS;
}
