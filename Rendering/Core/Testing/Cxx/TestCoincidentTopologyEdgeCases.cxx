// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercises VTK_RESOLVE_POLYGON_OFFSET in the situations where a depth offset
// is most likely to fail. Each viewport targets one failure mode:

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCylinderSource.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include "vtkRegressionTestImage.h"

#include <cmath>

namespace
{
constexpr double SurfaceColor[3] = { 1.0, 1.0, 0.3 };
constexpr double LineColor[3] = { 0.3, 0.3, 1.0 };
constexpr double PointColor[3] = { 1.0, 0.3, 1.0 };
constexpr double HiddenColor[3] = { 1.0, 0.0, 0.0 };

// Distance between a wall and the hidden geometry behind it.
constexpr double WallGap = 0.05;

//------------------------------------------------------------------------------
vtkActor* AddActor(vtkRenderer* renderer, vtkAlgorithm* source, int representation,
  const double color[3], float lineWidth = 1.0f, float pointSize = 4.0f)
{
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentation(representation);
  actor->GetProperty()->SetDiffuseColor(color[0], color[1], color[2]);
  actor->GetProperty()->SetLineWidth(lineWidth);
  actor->GetProperty()->SetPointSize(pointSize);
  actor->GetProperty()->LightingOff();
  renderer->AddActor(actor);
  return actor;
}

//------------------------------------------------------------------------------
void AddCoincident(
  vtkRenderer* renderer, vtkAlgorithm* source, float lineWidth = 1.0f, float pointSize = 4.0f)
{
  AddActor(renderer, source, VTK_POINTS, PointColor, lineWidth, pointSize);
  AddActor(renderer, source, VTK_WIREFRAME, LineColor, lineWidth, pointSize);
  AddActor(renderer, source, VTK_SURFACE, SurfaceColor, lineWidth, pointSize);
}

//------------------------------------------------------------------------------
vtkRenderer* AddViewport(vtkRenderWindow* renderWindow, int col, int row)
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetViewport(col / 3.0, (1 - row) / 2.0, (col + 1) / 3.0, (2 - row) / 2.0);
  renderer->SetBackground(0.2, 0.2, 0.2);
  renderWindow->AddRenderer(renderer);
  return renderer;
}

//------------------------------------------------------------------------------
// Looks at the origin from distance `r`, `angle` degrees away from the +z axis
// towards -y.
void SetCamera(vtkRenderer* renderer, double angle, double r, double nearPlane)
{
  const double theta = vtkMath::RadiansFromDegrees(angle);
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetFocalPoint(0.0, 0.0, 0.0);
  camera->SetPosition(0.0, -r * std::sin(theta), r * std::cos(theta));
  camera->SetViewUp(0.0, std::cos(theta), std::sin(theta));
  camera->SetClippingRange(nearPlane, 20.0);
}

//------------------------------------------------------------------------------
void SetupPlane(vtkPlaneSource* plane, double z, double halfSize = 1.0)
{
  plane->SetOrigin(-halfSize, -halfSize, z);
  plane->SetPoint1(halfSize, -halfSize, z);
  plane->SetPoint2(-halfSize, halfSize, z);
  plane->SetResolution(12, 12);
}

//------------------------------------------------------------------------------
void AddThinWall(vtkRenderer* renderer)
{
  vtkNew<vtkPlaneSource> back;
  SetupPlane(back, -WallGap, 0.8);
  vtkNew<vtkPlaneSource> front;
  SetupPlane(front, 0.0);
  AddActor(renderer, back, VTK_POINTS, HiddenColor);
  AddActor(renderer, back, VTK_WIREFRAME, HiddenColor);
  AddCoincident(renderer, front);
}
}

//------------------------------------------------------------------------------
int TestCoincidentTopologyEdgeCases(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(600, 400);
  renderWindow->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();

  // (0) grazing plane
  vtkNew<vtkPlaneSource> plane;
  SetupPlane(plane, 0.0);
  {
    vtkRenderer* renderer = AddViewport(renderWindow, 0, 0);
    AddCoincident(renderer, plane);
    SetCamera(renderer, 70.0, 3.0, 0.01);
  }

  // (1) grazing plane, wide lines and large points
  {
    vtkRenderer* renderer = AddViewport(renderWindow, 1, 0);
    AddCoincident(renderer, plane, 5.0f, 10.0f);
    SetCamera(renderer, 70.0, 3.0, 0.01);
  }

  // (2) sphere
  {
    vtkNew<vtkSphereSource> sphere;
    sphere->SetThetaResolution(24);
    sphere->SetPhiResolution(16);
    vtkRenderer* renderer = AddViewport(renderWindow, 2, 0);
    AddCoincident(renderer, sphere);
    SetCamera(renderer, 30.0, 2.5, 0.1);
  }

  // (3) thin wall, face-on
  {
    vtkRenderer* renderer = AddViewport(renderWindow, 0, 1);
    AddThinWall(renderer);
    SetCamera(renderer, 0.0, 4.0, 0.01);
  }

  // (4) thin wall, 60 degrees
  {
    vtkRenderer* renderer = AddViewport(renderWindow, 1, 1);
    AddThinWall(renderer);
    SetCamera(renderer, 60.0, 4.0, 0.01);
  }

  // (5) pipe with an inner wall, seen from the side. The pipe runs along y and
  // its ends are outside the view, so the inner wall is never directly visible.
  {
    vtkNew<vtkCylinderSource> outer;
    outer->SetRadius(1.0);
    outer->SetHeight(4.0);
    outer->SetResolution(32);
    vtkNew<vtkCylinderSource> inner;
    inner->SetRadius(1.0 - WallGap);
    inner->SetHeight(4.0);
    inner->SetResolution(32);
    vtkRenderer* renderer = AddViewport(renderWindow, 2, 1);
    AddActor(renderer, inner, VTK_POINTS, HiddenColor);
    AddActor(renderer, inner, VTK_WIREFRAME, HiddenColor);
    AddActor(renderer, inner, VTK_SURFACE, HiddenColor);
    AddCoincident(renderer, outer);
    SetCamera(renderer, 0.0, 4.5, 0.01);
  }

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
