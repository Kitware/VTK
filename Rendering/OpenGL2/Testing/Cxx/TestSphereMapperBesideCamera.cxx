// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Renders spheres whose centers lie exactly on the view x-axis, beside the camera.
// For those spheres, the direction from the center to the eye is parallel to the x-axis,
// which is the axis vtkOpenGLSphereMapper first uses to orient the quad around a sphere.
// The spheres are large enough to reach into the (wide) view, so they must still be drawn.
// A small sphere straight ahead of the camera is drawn for reference.

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

int TestSphereMapperBesideCamera(int argc, char* argv[])
{
  // centers and radii. the first two spheres are beside the camera, on the view x-axis.
  const double centers[3][3] = { { 3.0, 0.0, 0.0 }, { -3.0, 0.0, 0.0 }, { 0.0, 0.0, -6.0 } };
  const float radii[3] = { 2.5f, 2.5f, 1.0f };
  const float scalars[3] = { 0.0f, 1.0f, 2.0f };

  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> verts;
  vtkNew<vtkFloatArray> radiusArray;
  radiusArray->SetName("radii");
  vtkNew<vtkFloatArray> scalarArray;
  scalarArray->SetName("scalars");
  for (int i = 0; i < 3; ++i)
  {
    const vtkIdType id = points->InsertNextPoint(centers[i]);
    verts->InsertNextCell(1, &id);
    radiusArray->InsertNextValue(radii[i]);
    scalarArray->InsertNextValue(scalars[i]);
  }
  vtkNew<vtkPolyData> poly;
  poly->SetPoints(points);
  poly->SetVerts(verts);
  poly->GetPointData()->SetScalars(scalarArray);
  poly->GetPointData()->AddArray(radiusArray);

  vtkNew<vtkOpenGLSphereMapper> mapper;
  mapper->SetInputData(poly);
  mapper->SetScaleArray("radii");
  mapper->SetScalarRange(0.0, 2.0);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  // The headlight barely reaches the surfaces that face the camera from the side, so add
  // ambient light to show their colors. NaN normals or positions would render them black.
  actor->GetProperty()->SetAmbient(0.4);
  actor->GetProperty()->SetDiffuse(0.6);

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.1, 0.1, 0.15);
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 300);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // The camera sits at the origin and looks down -z, so world and view coordinates coincide.
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(0.0, 0.0, 0.0);
  camera->SetFocalPoint(0.0, 0.0, -1.0);
  camera->SetViewUp(0.0, 1.0, 0.0);
  camera->SetViewAngle(100.0);
  camera->SetClippingRange(0.05, 20.0);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
