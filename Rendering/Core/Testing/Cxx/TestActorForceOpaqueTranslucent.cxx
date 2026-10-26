// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkStringFormatter.h"
#include "vtkUnsignedCharArray.h"

#include <array>
#include <cstdlib>
#include <string>

namespace
{
bool CheckSphereDrawn(vtkRenderWindow* window, const char* label)
{
  vtkNew<vtkUnsignedCharArray> pixels;
  window->GetRGBACharPixelData(150, 150, 150, 150, /*front=*/1, pixels);
  const unsigned char r = pixels->GetValue(0);
  const unsigned char g = pixels->GetValue(1);
  const unsigned char b = pixels->GetValue(2);
  if (r < 40 && g < 40 && b < 40)
  {
    vtk::println("{}: sphere was not drawn, center pixel: {} {} {}", label, r, g, b);
    return false;
  }
  return true;
}

bool CheckState(vtkActor* actor, vtkRenderWindow* window, const char* label, bool expectOpaque,
  bool expectTranslucent)
{
  bool ok = true;
  // twice to check cache hits answer correctly.
  for (int i = 0; i < 2; ++i)
  {
    const bool opaque = actor->HasOpaqueGeometry() != 0;
    const bool translucent = actor->HasTranslucentPolygonalGeometry() != 0;
    if (opaque != expectOpaque || translucent != expectTranslucent)
    {
      vtk::println("{} (query {}): expected opaque={} translucent={}, got opaque={} translucent={}",
        label, i, expectOpaque, expectTranslucent, opaque, translucent);
      ok = false;
    }
  }
  // twice to check backends re-use proper graphics pipeline.
  window->Render();
  window->Render();
  return CheckSphereDrawn(window, label) && ok;
}

bool RunScenario(unsigned char alpha)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(32);
  sphere->SetPhiResolution(32);
  sphere->Update();

  vtkNew<vtkPolyData> poly;
  poly->ShallowCopy(sphere->GetOutput());

  // Direct RGBA scalars: red, with the given alpha.
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetName("colors");
  colors->SetNumberOfComponents(4);
  colors->SetNumberOfTuples(poly->GetNumberOfPoints());
  for (vtkIdType i = 0; i < colors->GetNumberOfTuples(); ++i)
  {
    colors->SetTypedTuple(i, std::array<unsigned char, 4>{ 255, 0, 0, alpha }.data());
  }
  poly->GetPointData()->SetScalars(colors);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(poly);
  mapper->SetColorModeToDefault();
  mapper->ScalarVisibilityOn();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetOpacity(1.0);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(300, 300);
  window->AddRenderer(renderer);
  renderer->ResetCamera();
  window->Render();

  const bool mapperOpaque = alpha == 255;
  const std::string prefix = mapperOpaque ? "opaque mapper" : "translucent mapper";
  bool ok = true;

  ok &= CheckState(actor, window, (prefix + ", no flags").c_str(), /*expectOpaque=*/mapperOpaque,
    /*expectTranslucent=*/!mapperOpaque);

  actor->ForceOpaqueOn();
  ok &= CheckState(actor, window, (prefix + ", ForceOpaque").c_str(), /*expectOpaque=*/true,
    /*expectTranslucent=*/false);

  actor->ForceOpaqueOff();
  actor->ForceTranslucentOn();
  ok &= CheckState(actor, window, (prefix + ", ForceTranslucent").c_str(), /*expectOpaque=*/false,
    /*expectTranslucent=*/true);

  actor->ForceTranslucentOff();
  ok &= CheckState(actor, window, (prefix + ", flags cleared").c_str(),
    /*expectOpaque=*/mapperOpaque, /*expectTranslucent=*/!mapperOpaque);

  actor->GetProperty()->SetOpacity(0.5);
  ok &= CheckState(actor, window, (prefix + ", property opacity 0.5").c_str(),
    /*expectOpaque=*/false, /*expectTranslucent=*/true);

  actor->GetProperty()->SetOpacity(1.0);
  ok &= CheckState(actor, window, (prefix + ", property opacity 1.0").c_str(),
    /*expectOpaque=*/mapperOpaque, /*expectTranslucent=*/!mapperOpaque);

  // Modify the mapper while the property is translucent, so that the mapper's
  // answer is recomputed in that state, then make the property opaque again.
  actor->GetProperty()->SetOpacity(0.5);
  mapper->Modified();
  ok &= CheckState(actor, window, (prefix + ", property opacity 0.5, mapper modified").c_str(),
    /*expectOpaque=*/false, /*expectTranslucent=*/true);

  actor->GetProperty()->SetOpacity(1.0);
  ok &= CheckState(actor, window, (prefix + ", property opacity 1.0 after mapper modified").c_str(),
    /*expectOpaque=*/mapperOpaque, /*expectTranslucent=*/!mapperOpaque);

  return ok;
}
}

int TestActorForceOpaqueTranslucent(int, char*[])
{
  bool ok = RunScenario(255);
  ok &= RunScenario(128);
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
