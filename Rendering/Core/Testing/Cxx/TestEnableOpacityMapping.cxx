// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkStringFormatter.h"
#include "vtkUnsignedCharArray.h"

#include <cstdlib>

namespace
{
bool CheckSphereDrawn(vtkRenderWindow* window)
{
  vtkNew<vtkUnsignedCharArray> pixels;
  window->GetRGBACharPixelData(150, 150, 150, 150, /*front=*/1, pixels);
  const unsigned char r = pixels->GetValue(0);
  const unsigned char g = pixels->GetValue(1);
  const unsigned char b = pixels->GetValue(2);
  vtk::println("center pixel: {} {} {}", r, g, b);

  if (r < 40 && g < 40 && b < 40)
  {
    vtk::println("BUG: sphere was not drawn");
    return false;
  }
  vtk::println("OK: sphere was drawn");
  return true;
}
}

int TestEnableOpacityMapping(int, char*[])
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(32);
  sphere->SetPhiResolution(32);
  sphere->Update();

  vtkNew<vtkPolyData> poly;
  poly->ShallowCopy(sphere->GetOutput());

  // float scalars, all 1.0.
  vtkNew<vtkFloatArray> alphas;
  alphas->SetName("alpha");
  alphas->SetNumberOfComponents(1);
  alphas->SetNumberOfTuples(poly->GetNumberOfPoints());
  alphas->Fill(1.0);
  poly->GetPointData()->SetScalars(alphas);

  vtkNew<vtkPiecewiseFunction> opacity;
  opacity->AddPoint(0.0, 0.0);
  opacity->AddPoint(1.0, 1.0);

  vtkNew<vtkDiscretizableColorTransferFunction> ctf;
  ctf->AddRGBPoint(0.0, 1.0, 0.0, 0.0);
  ctf->AddRGBPoint(1.0, 1.0, 0.0, 0.0);
  ctf->SetScalarOpacityFunction(opacity);
  ctf->EnableOpacityMappingOn();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(poly);
  mapper->SetLookupTable(ctf);
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarRange(0.0, 1.0);
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
  return CheckSphereDrawn(window) ? EXIT_SUCCESS : EXIT_FAILURE;
}
