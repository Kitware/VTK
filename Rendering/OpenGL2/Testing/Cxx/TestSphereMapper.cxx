// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercises vtkOpenGLSphereMapper in four viewports:
//   lower left:  perspective, per-point scale array, colors from mapped point scalars
//   lower right: parallel projection, same data
//   upper left:  uniform radius, colors from the actor's property with specular
//   upper right: actor with a rotation and translation

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkOpenGLSphereMapper.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

namespace
{
vtkNew<vtkPolyData> MakeLattice()
{
  constexpr int n = 4;
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> verts;
  vtkNew<vtkFloatArray> scalars;
  scalars->SetName("scalars");
  vtkNew<vtkFloatArray> radii;
  radii->SetName("radii");
  for (int k = 0; k < n; ++k)
  {
    for (int j = 0; j < n; ++j)
    {
      for (int i = 0; i < n; ++i)
      {
        const vtkIdType id = points->InsertNextPoint(i, j, k);
        verts->InsertNextCell(1, &id);
        scalars->InsertNextValue(static_cast<float>(i + j + k));
        radii->InsertNextValue(0.15f + 0.1f * static_cast<float>((i + 2 * j + 3 * k) % 4));
      }
    }
  }
  vtkNew<vtkPolyData> poly;
  poly->SetPoints(points);
  poly->SetVerts(verts);
  poly->GetPointData()->SetScalars(scalars);
  poly->GetPointData()->AddArray(radii);
  return poly;
}

void SetupCamera(vtkRenderer* renderer, bool parallel)
{
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetParallelProjection(parallel);
  camera->SetPosition(8.0, 6.0, 10.0);
  camera->SetFocalPoint(1.5, 1.5, 1.5);
  camera->SetViewUp(0.0, 1.0, 0.0);
  renderer->ResetCamera();
  // ResetCamera frames the sphere centers, zoom out so the whole spheres are visible.
  camera->Zoom(0.8);
  renderer->ResetCameraClippingRange();
}
}

int TestSphereMapper(int argc, char* argv[])
{
  auto lattice = ::MakeLattice();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  const double viewports[4][4] = {
    { 0.0, 0.0, 0.5, 0.5 },
    { 0.5, 0.0, 1.0, 0.5 },
    { 0.0, 0.5, 0.5, 1.0 },
    { 0.5, 0.5, 1.0, 1.0 },
  };

  for (int vp = 0; vp < 4; ++vp)
  {
    vtkNew<vtkOpenGLSphereMapper> mapper;
    mapper->SetInputData(lattice);
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(viewports[vp][0], viewports[vp][1], viewports[vp][2], viewports[vp][3]);
    renderer->SetBackground(0.1, 0.1, 0.15);
    renderer->AddActor(actor);
    renWin->AddRenderer(renderer);

    switch (vp)
    {
      case 0:
      case 1:
        mapper->SetScaleArray("radii");
        mapper->SetScalarRange(0.0, 9.0);
        break;
      case 2:
        mapper->ScalarVisibilityOff();
        mapper->SetRadius(0.35f);
        actor->GetProperty()->SetColor(0.9, 0.6, 0.2);
        actor->GetProperty()->SetSpecular(0.5);
        actor->GetProperty()->SetSpecularPower(40.0);
        break;
      case 3:
      default:
        mapper->SetScaleArray("radii");
        mapper->SetScalarRange(0.0, 9.0);
        actor->RotateY(30.0);
        actor->RotateX(15.0);
        actor->SetPosition(0.5, -0.5, 0.0);
        break;
    }
    ::SetupCamera(renderer, vp == 1);
  }

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
