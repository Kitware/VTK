// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Renders translucent spheres with vtkOpenGLSphereMapper. Translucent spheres are
// drawn twice (back halves, then front halves), so this covers the inverted-depth pass.

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

int TestSphereMapperTranslucent(int argc, char* argv[])
{
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> verts;
  vtkNew<vtkFloatArray> radii;
  radii->SetName("radii");
  for (int j = 0; j < 3; ++j)
  {
    for (int i = 0; i < 3; ++i)
    {
      const vtkIdType id = points->InsertNextPoint(i, j, 0.3 * (i + j));
      verts->InsertNextCell(1, &id);
      radii->InsertNextValue(0.45f + 0.1f * static_cast<float>((i + j) % 2));
    }
  }
  vtkNew<vtkPolyData> poly;
  poly->SetPoints(points);
  poly->SetVerts(verts);
  poly->GetPointData()->AddArray(radii);

  vtkNew<vtkOpenGLSphereMapper> mapper;
  mapper->SetInputData(poly);
  mapper->SetScaleArray("radii");
  mapper->ScalarVisibilityOff();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(0.3, 0.7, 1.0);
  actor->GetProperty()->SetOpacity(0.5);

  // an opaque sphere behind the translucent ones
  vtkNew<vtkPoints> backPoints;
  const vtkIdType backId = backPoints->InsertNextPoint(1.0, 1.0, -1.5);
  vtkNew<vtkCellArray> backVerts;
  backVerts->InsertNextCell(1, &backId);
  vtkNew<vtkPolyData> backPoly;
  backPoly->SetPoints(backPoints);
  backPoly->SetVerts(backVerts);
  vtkNew<vtkOpenGLSphereMapper> backMapper;
  backMapper->SetInputData(backPoly);
  backMapper->SetRadius(1.2f);
  vtkNew<vtkActor> backActor;
  backActor->SetMapper(backMapper);
  backActor->GetProperty()->SetColor(1.0, 0.4, 0.3);

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.1, 0.1, 0.15);
  renderer->AddActor(backActor);
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(1.0, 1.0, 8.0);
  camera->SetFocalPoint(1.0, 1.0, 0.0);
  camera->SetViewUp(0.0, 1.0, 0.0);
  renderer->ResetCamera();
  // ResetCamera frames the sphere centers, zoom out so the whole spheres are visible.
  camera->Zoom(0.8);
  renderer->ResetCameraClippingRange();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
